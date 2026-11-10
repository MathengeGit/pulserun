#pragma once
#include "game/player.h"
#include "raster/polygon.h"

enum class ObstacleType { Block, Spike, Saw, Orb, Pad, Portal };

// One level object in world pixels. Rectangular things (BLOCK, SPIKE, PAD) are stored by
// their top-left corner plus size; round things (SAW, ORB) and PORTAL by their centre.
struct Obstacle {
    ObstacleType type = ObstacleType::Block;
    float x = 0, y = 0;     // top-left (BLOCK/SPIKE/PAD) or centre (SAW/ORB/PORTAL)
    float w = 0, h = 0;     // size for BLOCK/SPIKE/PAD/PORTAL
    float radius = 0;       // SAW/ORB
    float speed = 0;        // SAW spin, in turns per second
    int target = 0;         // PORTAL: next level number (0 = none)

    Vec2 centre() const;
    AABB box() const;                  // broad-phase bounds
    Polygon hull(float time) const;    // exact collision shape at `time` seconds
    bool deadly() const { return type == ObstacleType::Spike || type == ObstacleType::Saw; }
    float sawAngle(float time) const { return time * speed * 6.2831853f; }
};

const char* obstacleName(ObstacleType t);
