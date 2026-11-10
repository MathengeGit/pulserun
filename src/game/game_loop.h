#pragma once
#include "core/window.h"
#include "game/game_state.h"
#include <cstdint>

// Fixed-timestep loop ("Fix Your Timestep", G. Fiedler): real time is accumulated and
// spent in exact FIXED_DT physics steps, then one frame is rendered with the leftover
// fraction as the interpolation alpha. Physics is therefore identical at 30, 60 or 144 Hz.
class GameLoop {
public:
    static constexpr double FIXED_DT = 1.0 / 60.0;  // 60 Hz physics
    static constexpr double MAX_FRAME = 0.25;       // clamp long stalls (tab switch, debugger)

    explicit GameLoop(int fbWidth = 960, int fbHeight = 540);

    // Runs until the window closes (desktop) or forever (browser: hands control to
    // emscripten_set_main_loop). Without a window it returns immediately.
    void run(Window& win, GameState& state);

    // One display frame: poll input, advance, render, present. Returns false to stop.
    bool frame(Window& win, GameState& state);

    // Advance by `realDt` seconds of wall time with input `in`; returns the number of fixed
    // updates run. Exposed so tests can drive the loop without a window.
    int advance(double realDt, GameState& state, const Input& in);

    float alpha() const { return (float)(accumulator_ / FIXED_DT); }
    uint64_t steps() const { return steps_; }
    Framebuffer& framebuffer() { return fb_; }

private:
    Framebuffer fb_;
    double accumulator_ = 0.0;
    double last_ = -1.0;
    uint64_t steps_ = 0;
    Input pending_;
};
