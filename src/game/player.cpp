#include "game/player.h"
#include <algorithm>
#include <cmath>

void Player::reset(Vec2 startCentre, float runSpeed) {
    pos = prev = startCentre;
    vel = Vec2(runSpeed, 0);
    on_ground = true;
    held = jumping = false;
    holdTime = 0;
    angle = prevAngle = 0;
}

bool Player::jump() {
    if (!on_ground) return false;
    vel.y = -JUMP_VEL;
    on_ground = false;
    jumping = true;
    holdTime = 0;
    return true;
}

void Player::launch(float upVel) {
    vel.y = -upVel;
    on_ground = false;
    jumping = false;  // orbs and pads give a fixed height
}

void Player::land(float surfaceY) {
    pos.y = surfaceY - SIZE / 2;
    vel.y = 0;
    on_ground = true;
    jumping = false;
}

void Player::update(float dt) {
    prev = pos;
    prevAngle = angle;
    const bool wasOnGround = on_ground;

    // Variable jump height: while the jump is held and still rising, gravity is reduced
    // for up to HOLD_MAX seconds. Releasing early ends the extension for this jump.
    float g = GRAVITY;
    if (jumping && held && holdTime < HOLD_MAX && vel.y < 0) {
        g *= HOLD_GRAVITY;
        holdTime += dt;
    } else if (!held) {
        jumping = false;
    }
    vel.y = std::min(vel.y + g * dt, TERMINAL_VEL);
    pos = pos + vel * dt;

    // Spin while airborne; on the ground ease to the nearest quarter turn so a face sits flat.
    if (!wasOnGround) {
        angle += SPIN * dt;
    } else {
        const float quarter = 3.14159265f / 2;
        const float target = std::round(angle / quarter) * quarter;
        const float step = 18.0f * dt;
        angle = std::fabs(target - angle) <= step ? target : angle + (target > angle ? step : -step);
    }
    on_ground = false;  // collision resolution decides whether we are still supported
}

Polygon Player::hull(float inset) const {
    const float h = SIZE / 2 - inset;
    return {Vec2(pos.x - h, pos.y - h), Vec2(pos.x + h, pos.y - h), Vec2(pos.x + h, pos.y + h),
            Vec2(pos.x - h, pos.y + h)};
}
