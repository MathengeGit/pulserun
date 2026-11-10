#pragma once
#include "game/level.h"
#include <cstdint>
#include <vector>

// Finds an input sequence that completes a level, by depth-first search over the only
// moments input matters: standing on a surface, or overlapping an unused orb. At each such
// moment it tries "wait", "tap" and "hold for the full 350 ms", and memoises dead ends.
// Because World is deterministic, a found plan replays exactly. Used by the tests (every
// shipped level must be beatable) and by the headless showcase renderer.
struct SolveResult {
    bool solved = false;
    std::vector<uint8_t> held;  // jump input per fixed step (pressed = rising edge)
    int nodes = 0;              // decision points explored
    float best = 0.0f;          // furthest progress reached (0..1)
};

SolveResult solveLevel(const Level& level, int maxNodes = 400000);

// Replays `held` from the start; returns the progress reached and whether it completed.
float replayPlan(const Level& level, const std::vector<uint8_t>& held, bool* completed = nullptr);
