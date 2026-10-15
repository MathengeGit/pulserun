#pragma once
#include "core/framebuffer.h"
#include "math/vec2.h"
#include <vector>

using Polygon = std::vector<Vec2>;

// Constructors for common shapes.
Polygon makeRect(float x, float y, float w, float h);
Polygon regularPolygon(Vec2 center, float radius, int sides, float rotation = 0.0f);
// Star / saw outline: alternates between outer and inner radius (2*points vertices).
Polygon starPolygon(Vec2 center, float outerR, float innerR, int points, float rotation = 0.0f);

// Outline (1px Bresenham edges) and thick outline (each edge is a filled quad).
void drawPolygon(Framebuffer& fb, const Polygon& p, Color c, bool closed = true);
void drawPolygonThick(Framebuffer& fb, const Polygon& p, float width, Color c, bool roundJoins = false,
                      bool closed = true);

// Scanline fill using an edge table + active edge table (even-odd rule).
// Works for concave polygons. Samples at pixel centres.
void fillPolygon(Framebuffer& fb, const Polygon& p, Color c);

// Ray-casting (even-odd) point-in-polygon test.
bool pointInPolygon(const Polygon& p, Vec2 pt);

// Scale about `pivot`, rotate about `pivot`, then translate. (2D transformation)
Polygon transformPolygon(const Polygon& p, Vec2 pivot, float angle, float scale, Vec2 translate);
