#pragma once
#include "game/difficulty.h"
#include "game/level.h"
#include <cstdint>
#include <vector>

class World;

// Finds an input sequence that completes a level, by depth-first search over the only
// moments input matters: standing on a surface, or overlapping an unused orb. At each such
// moment it tries "wait", "tap" and "hold for the full 350 ms", and memoises dead ends.
// Because World is deterministic, a found plan replays exactly. Used by the tests (every
// shipped level must be beatable on every difficulty) and by the headless showcase renderer.
struct SolveResult {
    bool solved = false;
    std::vector<uint8_t> held;  // jump input per fixed step (pressed = rising edge)
    int nodes = 0;              // decision points explored
    float best = 0.0f;          // furthest progress reached (0..1)
};

SolveResult solveLevel(const Level& level, int maxNodes = 400000, const Rules& rules = Rules{});
// Search from any world state until the level is complete or the cube passes `goalX`.
SolveResult solveFrom(const World& start, float goalX, int maxNodes);

// Replays `held` from the start; returns the progress reached and whether it completed.
float replayPlan(const Level& level, const std::vector<uint8_t>& held, bool* completed = nullptr,
                 const Rules& rules = Rules{});

// ---- Difficulty analysis ----
// For each jump press in a solved plan: how many fixed steps (1/60 s of real time) the press
// could move earlier or later and the run still survive the next stretch. A human needs a
// window of several frames; 1-2 frames is pixel-perfect. Used to tune the difficulty modes
// (pulserun --analyse) and by the tests.
struct TimingWindow {
    int step = 0;       // plan step of the press
    float x = 0;        // where in the level (world x)
    int early = 0;      // steps the press may come early...
    int late = 0;       // ...or late
    bool orb = false;   // the press fires an orb (mid-air) rather than a ground jump
    int frames() const { return early + late + 1; }
};
std::vector<TimingWindow> timingWindows(const Level& level, const Rules& rules, const std::vector<uint8_t>& plan,
                                        int maxShift = 18);
