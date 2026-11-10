#include "game/world.h"
#include <algorithm>

void World::setLevel(const Level* l) {
    level = l;
    maxExtent_ = 0;
    if (l)
        for (const Obstacle& o : l->obstacles) maxExtent_ = std::max(maxExtent_, o.box().x1 - o.x);
    reset();
}

void World::reset() {
    time = 0;
    dead = complete = false;
    killer = -1;
    steps = 0;
    buffer_ = 0;
    lastPad_ = -1;
    trail.clear();
    if (!level) return;
    player.reset(Vec2(level->startX(), level->groundY - Player::SIZE / 2), level->speed);
    orbUsed.assign(level->obstacles.size(), 0);
    padAge.assign(level->obstacles.size(), 99.0f);
}

float World::progress() const {
    if (!level) return 0;
    if (complete) return 1;
    const float span = level->endX() - level->startX();
    return span <= 0 ? 0 : std::clamp((player.pos.x - level->startX()) / span, 0.0f, 1.0f);
}

// Obstacles are sorted by x, so only a narrow window around the player needs testing.
void World::nearby(size_t& first, size_t& last) const {
    const auto& obs = level->obstacles;
    const float lo = player.pos.x - Player::SIZE - maxExtent_ - 8;
    const float hi = player.pos.x + Player::SIZE + 200;
    first = std::lower_bound(obs.begin(), obs.end(), lo, [](const Obstacle& o, float v) { return o.x < v; }) - obs.begin();
    last = std::upper_bound(obs.begin(), obs.end(), hi, [](float v, const Obstacle& o) { return v < o.x; }) - obs.begin();
}

bool World::touchingUnusedOrb() const {
    if (!level) return false;
    size_t a, b;
    nearby(a, b);
    for (size_t i = a; i < b; ++i) {
        const Obstacle& o = level->obstacles[i];
        if (o.type == ObstacleType::Orb && !orbUsed[i] && aabb_overlap(player.box(), aabbOf(o.hull(time)))) return true;
    }
    return false;
}

WorldEvents World::step(float dt, bool held, bool pressed) {
    WorldEvents ev;
    if (!level || dead || complete) return ev;
    time += dt;
    ++steps;
    if (pressed) buffer_ = JUMP_BUFFER;
    for (float& a : padAge) a += dt;

    player.setHeld(held);
    player.update(dt);

    size_t a, b;
    nearby(a, b);
    const std::vector<Obstacle> local(level->obstacles.begin() + a, level->obstacles.begin() + b);
    const CollisionResult r = resolve(player, local, level->groundY, time);
    ev.landed = r.landed;
    if (steps % 2 == 0) {
        trail.push_back(player.pos);
        if ((int)trail.size() > TRAIL) trail.erase(trail.begin());
    }
    if (r.dead) {
        dead = true;
        killer = (int)(a + r.killer);
        ev.died = true;
        ev.killer = killer;
        return ev;
    }
    if (r.portal || player.pos.x >= level->width) {
        complete = true;
        ev.completed = true;
        return ev;
    }

    const int pad = r.pad >= 0 ? (int)(a + r.pad) : -1;
    if (pad >= 0 && pad != lastPad_) {
        player.launch(PAD_VEL);
        padAge[pad] = 0;
        ev.pad = true;
    }
    lastPad_ = pad;

    const int orb = r.orb >= 0 ? (int)(a + r.orb) : -1;
    if (orb >= 0 && !orbUsed[orb] && buffer_ > 0) {
        player.launch(ORB_VEL);
        orbUsed[orb] = 1;
        buffer_ = 0;
        ev.orb = true;
    }

    // Jump from the ground on a (buffered) press, or keep jumping while the input is held.
    if (player.on_ground && (buffer_ > 0 || held)) {
        player.jump();
        buffer_ = 0;
        ev.jumped = true;
    }
    buffer_ = std::max(0.0f, buffer_ - dt);
    return ev;
}
