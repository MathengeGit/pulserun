#include "game/solver.h"
#include "game/game_loop.h"
#include "game/world.h"
#include <algorithm>
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

// ---- Difficulty analysis ----

namespace {

struct Press { int at, len; };

// Every world state along the plan: states[i] is the world before step i.
std::vector<World> statesOf(const Level& level, const Rules& rules, const std::vector<uint8_t>& plan) {
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
    return states;
}

// The press events: rising edges of `plan`, with their hold lengths.
std::vector<Press> pressesOf(const std::vector<uint8_t>& plan) {
    std::vector<Press> presses;
    for (int i = 0; i < (int)plan.size(); ++i)
        if (plan[i] && (i == 0 || !plan[i - 1])) {
            int n = 0;
            while (i + n < (int)plan.size() && plan[i + n]) ++n;
            presses.push_back({i, n});
        }
    return presses;
}

// The world just before press p fires `shift` steps away from where the plan has it
// (negative = earlier), or false if the cube dies idling first. An earlier press that would
// overlap the previous held jump cuts that hold short, as a player does (release for one
// step, then press): without this an orb press right after a full-length held jump could
// never move earlier, and its window read as 1-2 frames however wide it really is (#38).
bool shiftedStart(const std::vector<World>& states, const std::vector<Press>& presses, size_t p, int shift, World& out) {
    const Press pr = presses[p];
    if (shift < 0) {
        const int at = pr.at + shift;
        const int prevAt = p > 0 ? presses[p - 1].at : -1;
        const int prevEnd = p > 0 ? presses[p - 1].at + presses[p - 1].len : 0;
        if (at <= (p > 0 ? prevAt + 1 : 0)) return false;  // the previous press keeps a step
        out = states[at];
        if (at <= prevEnd) {  // still inside the previous hold: let go one step first
            out = states[at - 1];
            out.step(kDt, false, false);
        }
        return true;
    }
    out = states[pr.at];
    for (int j = 0; j < shift; ++j) {
        out.step(kDt, false, false);
        if (out.dead) return false;
    }
    return true;
}

// Hold for `len` steps from `t`: still alive, and can the run then clear the next stretch?
bool survivesPress(World t, int len, float goalX, int nodes, std::vector<uint8_t>* rest = nullptr) {
    bool pv = false;
    for (int j = 0; j < len && !t.dead && !t.complete; ++j) {
        t.step(kDt, true, !pv);
        pv = true;
    }
    if (t.dead) return false;
    const SolveResult s = solveFrom(t, goalX, nodes);
    if (rest) *rest = s.held;
    return s.solved;
}

TimingWindow measure(const Level& level, const std::vector<World>& states, const std::vector<Press>& presses,
                     size_t p, int planSize, int maxShift) {
    // Survive this far past the press: the next 1.3 s of world travel, so a shifted jump that
    // lands somewhere hopeless (e.g. straight in front of the next spike) does not count.
    const float horizon = level.speed * 1.3f;
    const Press pr = presses[p];
    TimingWindow win;
    win.step = pr.at;
    win.x = states[pr.at].player.pos.x;
    win.orb = !states[pr.at].player.on_ground;
    World t;
    for (int k = 1; k <= maxShift; ++k) {
        if (!shiftedStart(states, presses, p, -k, t) || !survivesPress(t, pr.len, win.x + horizon, 3000)) break;
        win.early = k;
    }
    // Later: the delayed press must start before the next press.
    const int nextAt = p + 1 < presses.size() ? presses[p + 1].at : planSize;
    for (int k = 1; k <= maxShift && pr.at + k + pr.len < nextAt; ++k) {
        if (!shiftedStart(states, presses, p, k, t) || !survivesPress(t, pr.len, win.x + horizon, 3000)) break;
        win.late = k;
    }
    return win;
}

}  // namespace

std::vector<TimingWindow> timingWindows(const Level& level, const Rules& rules, const std::vector<uint8_t>& plan,
                                        int maxShift) {
    const std::vector<World> states = statesOf(level, rules, plan);
    const std::vector<Press> presses = pressesOf(plan);
    std::vector<TimingWindow> out;
    for (size_t p = 0; p < presses.size(); ++p)
        out.push_back(measure(level, states, presses, p, (int)plan.size(), maxShift));
    return out;
}

std::vector<uint8_t> centrePlan(const Level& level, const Rules& rules, const std::vector<uint8_t>& plan, int maxShift) {
    // A move must not just push the problem onto the next press: score a plan at press p by
    // the smaller of the windows of p and p + 1, and keep a move only if that improves.
    auto score = [&](const std::vector<uint8_t>& pl, size_t p) {
        const std::vector<World> states = statesOf(level, rules, pl);
        const std::vector<Press> presses = pressesOf(pl);
        int s = 1 << 20;
        for (size_t q = p; q < presses.size() && q <= p + 1; ++q)
            s = std::min(s, measure(level, states, presses, q, (int)pl.size(), maxShift).frames());
        return s;
    };
    std::vector<uint8_t> out = plan;
    for (size_t p = 0;; ++p) {
        const std::vector<Press> presses = pressesOf(out);
        if (p >= presses.size()) break;
        const std::vector<World> states = statesOf(level, rules, out);
        const TimingWindow w = measure(level, states, presses, p, (int)out.size(), maxShift);
        const int centre = (w.late - w.early) / 2;  // steps to the middle of the window
        if (centre == 0) continue;
        int best = score(out, p);
        std::vector<uint8_t> bestPlan = out;
        for (int shift : {centre, centre / 2}) {
            World t;
            std::vector<uint8_t> rest;
            if (shift == 0 || !shiftedStart(states, presses, p, shift, t) ||
                !survivesPress(t, presses[p].len, 1e30f, 400000, &rest))
                continue;  // cannot finish the level from the moved press
            const int at = presses[p].at + shift;
            // Inputs up to the new press: the plan's own, then idle steps for a later press, or
            // a one-step release when an earlier press cuts the previous hold short.
            std::vector<uint8_t> next(out.begin(), out.begin() + std::min(at, presses[p].at));
            next.resize(at, 0);
            if (shift < 0 && at > 0) next[at - 1] = 0;
            next.insert(next.end(), presses[p].len, 1);
            next.insert(next.end(), rest.begin(), rest.end());
            bool completed = false;
            if (replayPlan(level, next, &completed, rules) < 1.0f || !completed) continue;
            const int s = score(next, p);
            if (s > best) {
                best = s;
                bestPlan = next;
            }
        }
        out = bestPlan;
    }
    return out;
}
