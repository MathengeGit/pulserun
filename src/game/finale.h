#pragma once
#include "core/framebuffer.h"
#include "game/sprites.h"
#include <string>
#include <vector>

// The "game beaten" sequence shown after the last level. Every frame is built from the
// project's own raster code: Bezier and Catmull-Rom curves, midpoint circles, scanline
// polygon fill, Cohen-Sutherland / Sutherland-Hodgman clipping, Mat3 transforms and the
// 5x7 bitmap font.
struct FinaleStats {
    std::string difficulty;               // "EASY", "NORMAL" or "HARD"
    int totalAttempts = 0;                // across all levels
    float totalTime = 0;                  // sum of best completion times, seconds
    std::vector<std::string> levelNames;  // e.g. PRIMER, ESCALATION, ADVANCED
    std::vector<float> levelTimes;        // best time per level, seconds
    std::vector<int> levelAttempts;       // attempts per level
};

// t = seconds since the finale started. A pure function of (t, stats, pal): no hidden
// state, so the headless video recorder can render any frame deterministically.
void drawFinale(Framebuffer& fb, float t, const FinaleStats& stats, const Palette& pal);

// Seconds until the sequence reaches its resting "credits / buttons" state. After this the
// picture keeps animating gently (heartbeat, fireworks, credit pages) but the layout is fixed.
float finaleIntroDuration();

// Screen rectangle reserved for two vertically stacked buttons once t >= finaleIntroDuration().
void finaleButtonArea(int& x, int& y, int& w, int& h);
