#pragma once
#include "math/vec2.h"
#include "raster/polygon.h"

// Axis-aligned bounding box (x0,y0) top-left to (x1,y1) bottom-right. Used as the broad phase.
struct AABB {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

// The cube. Positions are the centre of the cube in world pixels (y grows downward).
struct Player {
    static constexpr float SIZE = 48.0f;          // hitbox edge (the sprite is drawn at 56)
    static constexpr float GRAVITY = 1200.0f;      // px/s^2
    static constexpr float TERMINAL_VEL = 900.0f;  // px/s, max fall speed
    static constexpr float JUMP_VEL = 600.0f;      // px/s, initial upward speed of a jump
    static constexpr float HOLD_MAX = 0.35f;       // s, how long holding can extend a jump
    static constexpr float HOLD_GRAVITY = 0.6f;    // gravity multiplier while a held jump rises
    static constexpr float SPIN = 3.14159265f;     // rad/s in the air (half a turn per second)

    Vec2 pos, prev;  // centre now and at the previous step (for interpolation and landing)
    Vec2 vel;
    bool on_ground = false;
    bool held = false;      // jump input is held this step
    bool jumping = false;   // rising from a jump that holding may still extend
    float holdTime = 0.0f;  // how long the current jump has been extended
    float angle = 0.0f, prevAngle = 0.0f;

    void reset(Vec2 startCentre, float runSpeed);
    void setHeld(bool h) { held = h; }
    bool jump();               // vel.y = -600 if on the ground; returns whether it jumped
    void launch(float upVel);  // orb / pad: set an upward speed even in mid-air
    void land(float surfaceY); // put the bottom on `surfaceY`, stop falling, mark on_ground
    void update(float dt);     // gravity (reduced while a held jump rises), terminal velocity, spin

    float bottom() const { return pos.y + SIZE / 2; }
    float prevBottom() const { return prev.y + SIZE / 2; }
    AABB box() const { return {pos.x - SIZE / 2, pos.y - SIZE / 2, pos.x + SIZE / 2, pos.y + SIZE / 2}; }
    // Collision polygon: the unrotated square, shrunk by `inset` on every side.
    Polygon hull(float inset = 0.0f) const;
};
