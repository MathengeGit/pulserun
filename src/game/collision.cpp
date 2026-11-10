#include "game/collision.h"
#include <algorithm>

// ---------- Obstacle shapes ----------

Vec2 Obstacle::centre() const {
    switch (type) {
    case ObstacleType::Saw:
    case ObstacleType::Orb:
    case ObstacleType::Portal: return Vec2(x, y);
    default: return Vec2(x + w / 2, y + h / 2);
    }
}

AABB Obstacle::box() const {
    switch (type) {
    case ObstacleType::Saw:
    case ObstacleType::Orb: return {x - radius, y - radius, x + radius, y + radius};
    case ObstacleType::Portal: return {x - w / 2, y - h / 2, x + w / 2, y + h / 2};
    default: return {x, y, x + w, y + h};
    }
}

Polygon Obstacle::hull(float time) const {
    switch (type) {
    case ObstacleType::Spike:
        // Forgiving hitbox: the middle of the triangle, not its full base (as in most runners).
        return {Vec2(x + w * 0.22f, y + h), Vec2(x + w * 0.5f, y + h * 0.22f), Vec2(x + w * 0.78f, y + h)};
    case ObstacleType::Saw: return regularPolygon(Vec2(x, y), radius * 0.82f, 12, sawAngle(time));
    case ObstacleType::Orb: {
        const float r = radius * 2.0f;  // generous trigger zone around the orb
        return makeRect(x - r, y - r, 2 * r, 2 * r);
    }
    case ObstacleType::Portal: return makeRect(x - w / 2, y - h / 2, w, h);
    default: return makeRect(x, y, w, h);
    }
}

const char* obstacleName(ObstacleType t) {
    switch (t) {
    case ObstacleType::Block: return "BLOCK";
    case ObstacleType::Spike: return "SPIKE";
    case ObstacleType::Saw: return "SAW";
    case ObstacleType::Orb: return "ORB";
    case ObstacleType::Pad: return "PAD";
    case ObstacleType::Portal: return "PORTAL";
    }
    return "?";
}

// ---------- Geometry ----------

AABB aabbOf(const Polygon& p) {
    AABB b{1e30f, 1e30f, -1e30f, -1e30f};
    for (const Vec2& v : p) {
        b.x0 = std::min(b.x0, v.x);
        b.y0 = std::min(b.y0, v.y);
        b.x1 = std::max(b.x1, v.x);
        b.y1 = std::max(b.y1, v.y);
    }
    return b;
}

bool aabb_overlap(const AABB& a, const AABB& b) {
    return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}

namespace {
float cross(Vec2 o, Vec2 a, Vec2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }
bool onSegment(Vec2 p, Vec2 q, Vec2 r) {  // q collinear with p-r: is it within the box?
    return std::min(p.x, r.x) <= q.x && q.x <= std::max(p.x, r.x) && std::min(p.y, r.y) <= q.y &&
           q.y <= std::max(p.y, r.y);
}
int sign(float v) { return (v > 1e-6f) - (v < -1e-6f); }
}  // namespace

bool segments_intersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    const int d1 = sign(cross(c, d, a)), d2 = sign(cross(c, d, b));
    const int d3 = sign(cross(a, b, c)), d4 = sign(cross(a, b, d));
    if (d1 * d2 < 0 && d3 * d4 < 0) return true;
    if (d1 == 0 && onSegment(c, a, d)) return true;
    if (d2 == 0 && onSegment(c, b, d)) return true;
    if (d3 == 0 && onSegment(a, c, b)) return true;
    if (d4 == 0 && onSegment(a, d, b)) return true;
    return false;
}

bool polygon_overlap(const Polygon& a, const Polygon& b) {
    if (a.size() < 3 || b.size() < 3) return false;
    if (!aabb_overlap(aabbOf(a), aabbOf(b))) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        const Vec2 a0 = a[i], a1 = a[(i + 1) % a.size()];
        for (size_t j = 0; j < b.size(); ++j)
            if (segments_intersect(a0, a1, b[j], b[(j + 1) % b.size()])) return true;
    }
    // No edges cross: either disjoint, or one contains the other.
    return pointInPolygon(b, a[0]) || pointInPolygon(a, b[0]);
}

// ---------- Resolution ----------

CollisionResult resolve(Player& player, const std::vector<Obstacle>& obs, float groundY, float time) {
    CollisionResult res;
    if (player.bottom() >= groundY) {
        player.land(groundY);
        res.landed = true;
    }

    // Blocks first, so standing on one is settled before hazards are tested.
    for (size_t i = 0; i < obs.size(); ++i) {
        const Obstacle& o = obs[i];
        if (o.type != ObstacleType::Block) continue;
        const AABB b = o.box();
        if (!aabb_overlap(player.box(), b)) continue;
        if (player.vel.y >= 0 && player.prevBottom() <= b.y0 + kLandTolerance) {
            player.land(b.y0);  // came down onto the top face
            res.landed = true;
        } else {
            res.dead = true;  // hit the side or the underside
            res.killer = (int)i;
            return res;
        }
    }

    const AABB pb = player.box();
    const Polygon ph = player.hull(2.0f);
    for (size_t i = 0; i < obs.size(); ++i) {
        const Obstacle& o = obs[i];
        if (o.type == ObstacleType::Block) continue;
        const AABB broad = (o.type == ObstacleType::Orb) ? aabbOf(o.hull(time)) : o.box();
        if (!aabb_overlap(pb, broad)) continue;  // broad phase
        switch (o.type) {
        case ObstacleType::Spike:
        case ObstacleType::Saw:
            if (polygon_overlap(ph, o.hull(time))) {  // narrow phase
                res.dead = true;
                res.killer = (int)i;
                return res;
            }
            break;
        case ObstacleType::Orb: res.orb = (int)i; break;
        case ObstacleType::Pad: res.pad = (int)i; break;
        case ObstacleType::Portal:
            res.portal = true;
            res.portalTarget = o.target;
            break;
        default: break;
        }
    }
    return res;
}
