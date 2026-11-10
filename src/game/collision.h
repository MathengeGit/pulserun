#pragma once
#include "game/obstacle.h"
#include "game/player.h"
#include <vector>

// ---- Geometry tests ----
AABB aabbOf(const Polygon& p);
// Broad phase. Touching edges do not count as overlap (a cube resting on a block is not inside it).
bool aabb_overlap(const AABB& a, const AABB& b);
// Proper segment intersection (orientation tests; collinear overlaps count).
bool segments_intersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d);
// Narrow phase for any simple polygons, convex or concave: they overlap if any pair of edges
// cross, or if one polygon lies entirely inside the other (checked with pointInPolygon).
bool polygon_overlap(const Polygon& a, const Polygon& b);

// ---- Resolution ----
struct CollisionResult {
    bool dead = false;     // touched a spike or saw, or hit a block from the side or below
    bool landed = false;   // standing on the ground or a block top this step
    int killer = -1;       // index of the obstacle that killed the player
    int orb = -1;          // index of an orb the player overlaps
    int pad = -1;          // index of a jump pad the player touches
    bool portal = false;   // reached a portal
    int portalTarget = 0;
};

// How far below a block's top the cube's bottom may have been on the previous step and still
// count as landing on it (one step of fast falling, plus a little forgiveness).
constexpr float kLandTolerance = 10.0f;

// Resolves the player against the ground (y = groundY) and every obstacle after a physics
// step. Landing snaps the player onto the surface and sets on_ground.
CollisionResult resolve(Player& player, const std::vector<Obstacle>& obs, float groundY, float time);
