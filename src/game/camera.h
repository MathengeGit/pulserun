#pragma once
#include "math/mat3.h"
#include <algorithm>

// 2D camera. World space is the level's coordinate system (pixels, y down); the camera
// is a single homogeneous transform that every world-space vertex goes through before
// it is rasterised:  screen = world_to_screen() * world.
struct Camera {
    float scroll_x = 0.0f;  // world x at the left edge of the screen
    float scroll_y = 0.0f;  // world y at the top edge of the screen
    float shake_x = 0.0f, shake_y = 0.0f;  // screen-space offset (death shake)

    // translate(-scroll) moves the world under a fixed screen; the shake is applied after,
    // in screen space, so it never changes what the physics sees.
    Mat3 world_to_screen() const { return Mat3::translate(shake_x, shake_y) * Mat3::translate(-scroll_x, -scroll_y); }
    Mat3 screen_to_world() const { return world_to_screen().inverse(); }
    Vec2 toScreen(Vec2 world) const { return world_to_screen() * world; }

    // Keep the player `playerScreenX` pixels from the left edge. Vertically, keep the level's
    // ground at `groundScreenY` unless the player climbs within `topMargin` of the top, then
    // ease upward (and back down) so high jumps stay on screen.
    void follow(Vec2 player, float groundWorldY, float dt, float playerScreenX = 260.0f,
                float groundScreenY = 440.0f, float topMargin = 150.0f) {
        scroll_x = player.x - playerScreenX;
        const float base = groundWorldY - groundScreenY;
        const float target = std::min(base, player.y - topMargin);
        scroll_y += (target - scroll_y) * std::min(1.0f, dt * 6.0f);
    }

    // Snap to the resting position for a level start.
    void reset(Vec2 player, float groundWorldY, float playerScreenX = 260.0f, float groundScreenY = 440.0f) {
        scroll_x = player.x - playerScreenX;
        scroll_y = groundWorldY - groundScreenY;
        shake_x = shake_y = 0.0f;
    }

    static Camera lerp(const Camera& a, const Camera& b, float t) {
        Camera c = b;
        c.scroll_x = a.scroll_x + (b.scroll_x - a.scroll_x) * t;
        c.scroll_y = a.scroll_y + (b.scroll_y - a.scroll_y) * t;
        return c;
    }
};
