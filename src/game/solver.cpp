#include "game/solver.h"
#include "game/game_loop.h"
#include "game/world.h"
#include <cmath>
#include <unordered_set>

namespace {

const float kDt = (float)GameLoop::FIXED_DT;
const int kHoldSteps = (int)std::ceil(Player::HOLD_MAX / kDt);

struct Search {
    float goalX;
    int maxNodes;
    SolveResult res;
    std::vector<uint8_t> path;
    std::unordered_set<uint64_t> deadEnds;

    bool done(const World& w) const { return w.complete || w.player.pos.x >= goalX; }
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
        for (int i = 0; i < n && !w.dead && !done(w); ++i) {
            w.step(kDt, held, held && !prevHeld);
            path.push_back(held);
            prevHeld = held;
        }
        while (!w.dead && !done(w) && !decisionPoint(w)) {
            w.step(kDt, false, false);
            path.push_back(0);
        }
        res.best = std::max(res.best, w.progress());
        return !w.dead;
    }

    bool dfs(const World& w) {
        if (done(w)) return true;
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

SolveResult solveFrom(const World& start, float goalX, int maxNodes) {
    Search s{goalX, maxNodes, {}, {}, {}};
    s.res.solved = s.dfs(start);
    if (s.res.solved) s.res.held = s.path;
    return s.res;
}

SolveResult solveLevel(const Level& level, int maxNodes, const Rules& rules) {
    World w(&level);
    w.setRules(rules);
    return solveFrom(w, 1e30f, maxNodes);
}

float replayPlan(const Level& level, const std::vector<uint8_t>& held, bool* completed, const Rules& rules) {
    World w(&level);
    w.setRules(rules);
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

std::vector<TimingWindow> timingWindows(const Level& level, const Rules& rules, const std::vector<uint8_t>& plan,
                                        int maxShift) {
    // Every world state along the plan: states[i] is the world before step i.
    std::vector<World> states;
    states.reserve(plan.size() + 1);
    World w(&level);
    w.setRules(rules);
    bool prev = false;
    for (uint8_t h : plan) {
        states.push_back(w);
        w.step(kDt, h != 0, h && !prev);
        prev = h != 0;
    }
    states.push_back(w);

    // The press events: rising edges of `plan`, with their hold lengths.
    struct Press { int at, len; };
    std::vector<Press> presses;
    for (int i = 0; i < (int)plan.size(); ++i)
        if (plan[i] && (i == 0 || !plan[i - 1])) {
            int n = 0;
            while (i + n < (int)plan.size() && plan[i + n]) ++n;
            presses.push_back({i, n});
        }

    // Survive this far past the press: the next 1.3 s of world travel, so a shifted jump that
    // lands somewhere hopeless (e.g. straight in front of the next spike) does not count.
    const float horizon = level.speed * 1.3f;
    // Does pressing at step `at` (holding `len` steps) still lead to a survivable state?
    auto survives = [&](World t, int len, float fromX) {
        bool pv = false;
        for (int j = 0; j < len && !t.dead && !t.complete; ++j) {
            t.step(kDt, true, !pv);
            pv = true;
        }
        if (t.dead) return false;
        return solveFrom(t, fromX + horizon, 3000).solved;
    };

    std::vector<TimingWindow> out;
    for (size_t p = 0; p < presses.size(); ++p) {
        const Press pr = presses[p];
        TimingWindow win;
        win.step = pr.at;
        win.x = states[pr.at].player.pos.x;
        win.orb = !states[pr.at].player.on_ground;
        // Earlier: the preceding steps must be idle (no overlap with the previous press).
        const int prevEnd = p > 0 ? presses[p - 1].at + presses[p - 1].len : 0;
        for (int k = 1; k <= maxShift && pr.at - k > prevEnd; ++k) {
            if (!survives(states[pr.at - k], pr.len, win.x)) break;
            win.early = k;
        }
        // Later: the delayed press must start before the cube dies idling, and before the next press.
        const int nextAt = p + 1 < presses.size() ? presses[p + 1].at : (int)plan.size();
        for (int k = 1; k <= maxShift && pr.at + k + pr.len < nextAt; ++k) {
            World t = states[pr.at];
            bool alive = true;
            for (int j = 0; j < k && alive; ++j) {
                t.step(kDt, false, false);
                alive = !t.dead;
            }
            if (!alive || !survives(t, pr.len, win.x)) break;
            win.late = k;
        }
        out.push_back(win);
    }
    return out;
}
