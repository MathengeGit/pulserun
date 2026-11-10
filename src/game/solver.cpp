#include "game/solver.h"
#include "game/game_loop.h"
#include "game/world.h"
#include <cmath>
#include <unordered_set>

namespace {

const float kDt = (float)GameLoop::FIXED_DT;
const int kHoldSteps = (int)std::ceil(Player::HOLD_MAX / kDt);

struct Search {
    const Level& level;
    int maxNodes;
    SolveResult res;
    std::vector<uint8_t> path;
    std::unordered_set<uint64_t> deadEnds;

    bool decisionPoint(const World& w) const { return w.player.on_ground || w.touchingUnusedOrb(); }

    uint64_t key(const World& w) const {
        int used = 0;
        for (char u : w.orbUsed) used += u;
        const uint64_t y = (uint64_t)(int64_t)std::lround(w.player.pos.y * 2) & 0xFFFF;
        const uint64_t vy = (uint64_t)(int64_t)std::lround(w.player.vel.y) & 0xFFF;
        return ((uint64_t)w.steps << 34) ^ (y << 18) ^ (vy << 6) ^ ((uint64_t)w.player.on_ground << 5) ^ (uint64_t)(used & 31);
    }

    // Feed `n` steps of `held`, then idle until the next decision point. False if it died.
    bool run(World& w, bool held, int n) {
        bool prevHeld = !path.empty() && path.back();
        for (int i = 0; i < n && !w.dead && !w.complete; ++i) {
            w.step(kDt, held, held && !prevHeld);
            path.push_back(held);
            prevHeld = held;
        }
        while (!w.dead && !w.complete && !decisionPoint(w)) {
            w.step(kDt, false, false);
            path.push_back(0);
        }
        res.best = std::max(res.best, w.progress());
        return !w.dead;
    }

    bool dfs(const World& w) {
        if (w.complete) return true;
        if (++res.nodes > maxNodes) return false;
        const uint64_t k = key(w);
        if (deadEnds.count(k)) return false;
        const size_t mark = path.size();
        // Options in order: wait one step, tap (1 step), hold for the full extension.
        const struct { bool held; int steps; } opts[] = {{false, 1}, {true, 1}, {true, kHoldSteps}};
        const bool lastHeld = !path.empty() && path.back();
        for (const auto& o : opts) {
            if (o.held && lastHeld) continue;  // need a release before the next press
            World next = w;
            if (run(next, o.held, o.steps) && dfs(next)) return true;
            path.resize(mark);
            if (res.nodes > maxNodes) return false;
        }
        deadEnds.insert(k);
        return false;
    }
};

}  // namespace

SolveResult solveLevel(const Level& level, int maxNodes) {
    Search s{level, maxNodes, {}, {}, {}};
    World w(&level);
    s.res.solved = s.dfs(w);
    if (s.res.solved) s.res.held = s.path;
    return s.res;
}

float replayPlan(const Level& level, const std::vector<uint8_t>& held, bool* completed) {
    World w(&level);
    bool prev = false;
    for (uint8_t h : held) {
        w.step(kDt, h != 0, h && !prev);
        prev = h != 0;
        if (w.dead || w.complete) break;
    }
    // Let it run out (the plan ends at the moment of completion).
    for (int i = 0; i < 600 && !w.dead && !w.complete; ++i) w.step(kDt, false, false);
    if (completed) *completed = w.complete;
    return w.progress();
}
