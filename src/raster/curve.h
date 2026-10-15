#pragma once
#include "core/framebuffer.h"
#include "raster/polygon.h"
#include <vector>

// Cubic Bezier evaluated with De Casteljau's algorithm (repeated linear interpolation).
Vec2 bezierPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t);

// Every intermediate level of the De Casteljau construction at parameter t:
// levels[0] = 3 points, levels[1] = 2 points, levels[2] = 1 point (on the curve).
std::vector<std::vector<Vec2>> deCasteljauLevels(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t);

std::vector<Vec2> sampleBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int segments, float tEnd = 1.0f);

// Catmull-Rom spline (passes through every control point).
Vec2 catmullRomPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t);
std::vector<Vec2> sampleCatmullRom(const std::vector<Vec2>& pts, int segmentsPerSpan);

// Drawing helpers.
void drawPolylineThick(Framebuffer& fb, const std::vector<Vec2>& pts, float width, Color c, bool roundJoins = true);
void drawBezier(Framebuffer& fb, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float width, Color c, int segments = 64);
