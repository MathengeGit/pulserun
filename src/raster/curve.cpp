#include "raster/curve.h"
#include "raster/circle.h"
#include "raster/line.h"
#include <algorithm>
#include <cmath>

Vec2 bezierPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    const auto lv = deCasteljauLevels(p0, p1, p2, p3, t);
    return lv.back()[0];
}

std::vector<std::vector<Vec2>> deCasteljauLevels(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    std::vector<std::vector<Vec2>> levels;
    std::vector<Vec2> cur = {p0, p1, p2, p3};
    while (cur.size() > 1) {
        std::vector<Vec2> next;
        for (size_t i = 0; i + 1 < cur.size(); ++i) next.push_back(lerp(cur[i], cur[i + 1], t));
        levels.push_back(next);
        cur = next;
    }
    return levels;
}

std::vector<Vec2> sampleBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, int segments, float tEnd) {
    std::vector<Vec2> pts;
    for (int i = 0; i <= segments; ++i) pts.push_back(bezierPoint(p0, p1, p2, p3, tEnd * i / segments));
    return pts;
}

Vec2 catmullRomPoint(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    const float t2 = t * t, t3 = t2 * t;
    return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
            (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
}

std::vector<Vec2> sampleCatmullRom(const std::vector<Vec2>& pts, int segmentsPerSpan) {
    std::vector<Vec2> out;
    const int n = (int)pts.size();
    if (n < 2) return pts;
    for (int i = 0; i < n - 1; ++i) {
        const Vec2 p0 = pts[std::max(i - 1, 0)], p1 = pts[i], p2 = pts[i + 1], p3 = pts[std::min(i + 2, n - 1)];
        for (int s = 0; s < segmentsPerSpan; ++s) out.push_back(catmullRomPoint(p0, p1, p2, p3, (float)s / segmentsPerSpan));
    }
    out.push_back(pts.back());
    return out;
}

void drawPolylineThick(Framebuffer& fb, const std::vector<Vec2>& pts, float width, Color c, bool roundJoins) {
    if (pts.size() < 2) return;
    for (size_t i = 0; i + 1 < pts.size(); ++i) drawThickLine(fb, pts[i], pts[i + 1], width, c);
    if (roundJoins) {
        const int r = std::max(1, (int)(width * 0.5f));  // floor: disc diameter 2r+1 matches the stroke
        for (const Vec2& v : pts) fillCircle(fb, (int)std::lround(v.x), (int)std::lround(v.y), r, c);
    }
}

void drawBezier(Framebuffer& fb, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float width, Color c, int segments) {
    drawPolylineThick(fb, sampleBezier(p0, p1, p2, p3, segments), width, c);
}
