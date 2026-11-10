#pragma once
#include "core/framebuffer.h"
#include "core/input.h"

// Anything the game loop can drive: a fixed-rate update and a render that may
// interpolate between the last two updates.
class GameState {
public:
    virtual ~GameState() = default;
    // One fixed step of `dt` seconds. `in` carries held keys plus any edges since the last step.
    virtual void update(float dt, const Input& in) = 0;
    // Draw the current state. `alpha` in [0, 1) is how far real time is past the last update.
    virtual void render(Framebuffer& fb, float alpha) = 0;
    virtual bool wantsQuit() const { return false; }
};
