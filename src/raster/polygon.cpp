#include "raster/polygon.h"
#include "raster/circle.h"
#include "raster/line.h"
#include <algorithm>
#include <cmath>

Polygon makeRect(float x, float y, float w, float h) {
    return Polygon{Vec2(x, y), Vec2(x + w, y), Vec2(x + w, y + h), Vec2(x, y + h)};
}

Polygon regularPolygon(Vec2 center, float radius, int sides, float rotation) {
    Polygon p;
    for (int i = 0; i < sides; ++i) {
        const float a = rotation + 2.0f * kPi * i / sides - kPi / 2;  // first vertex points up
        p.push_back(center + Vec2(std::cos(a), std::sin(a)) * radius);
    }
    return p;
}

Polygon starPolygon(Vec2 center, float outerR, float innerR, int points, float rotation) {
    Polygon p;
    for (int i = 0; i < points * 2; ++i) {
        const float a = rotation + kPi * i / points - kPi / 2;
        const float r = (i % 2 == 0) ? outerR : innerR;
        p.push_back(center + Vec2(std::cos(a), std::sin(a)) * r);
    }
    return p;
}

void drawPolygon(Framebuffer& fb, const Polygon& p, Color c, bool closed) {
    const size_t n = p.size();
    if (n < 2) return;
    const size_t edges = closed ? n : n - 1;
    for (size_t i = 0; i < edges; ++i) {
        const Vec2 a = p[i], b = p[(i + 1) % n];
        drawLineBresenham(fb, (int)std::lround(a.x), (int)std::lround(a.y), (int)std::lround(b.x),
                          (int)std::lround(b.y), c);
    }
}

void drawPolygonThick(Framebuffer& fb, const Polygon& p, float width, Color c, bool roundJoins, bool closed) {
    const size_t n = p.size();
    if (n < 2) return;
    const size_t edges = closed ? n : n - 1;
    for (size_t i = 0; i < edges; ++i) drawThickLine(fb, p[i], p[(i + 1) % n], width, c);
    if (roundJoins) {
        const int r = std::max(1, (int)(width * 0.5f));  // floor: disc diameter 2r+1 matches the stroke
        for (const Vec2& v : p) fillCircle(fb, (int)std::lround(v.x), (int)std::lround(v.y), r, c);
    }
}

// Scanline polygon fill.
//  1. Edge table (ET): every non-horizontal edge, sorted by its top y.
//  2. For each scanline, move edges whose top has been reached into the
//     active edge table (AET) and drop edges whose bottom has passed.
//  3. Sort the AET by x; pixels between pairs of crossings are inside.
//  4. Each active edge's x advances by dx/dy (its inverse slope) per scanline.
void fillPolygon(Framebuffer& fb, const Polygon& poly, Color c) {
    const size_t n = poly.size();
    if (n < 3) return;

    struct Edge {
        float yMin, yMax, xAtYMin, invSlope;
    };
    struct Active {
        float yMax, x, invSlope;
    };

    std::vector<Edge> et;
    float minY = 1e30f, maxY = -1e30f;
    for (size_t i = 0; i < n; ++i) {
        Vec2 a = poly[i], b = poly[(i + 1) % n];
        minY = std::min(minY, a.y);
        maxY = std::max(maxY, a.y);
        if (a.y == b.y) continue;  // horizontal edges never cross a scanline centre
        if (a.y > b.y) std::swap(a, b);
        et.push_back({a.y, b.y, a.x, (b.x - a.x) / (b.y - a.y)});
    }
    std::sort(et.begin(), et.end(), [](const Edge& l, const Edge& r) { return l.yMin < r.yMin; });

    int yStart = (int)std::ceil(minY - 0.5f);
    int yEnd = (int)std::ceil(maxY - 0.5f) - 1;  // scanline centres y+0.5 in [minY, maxY)
    yStart = std::max(yStart, 0);
    yEnd = std::min(yEnd, fb.height() - 1);

    std::vector<Active> aet;
    size_t next = 0;
    // Edges that start above the visible area still need to enter the AET.
    for (int y = (int)std::ceil(minY - 0.5f); y <= yEnd; ++y) {
        const float sy = y + 0.5f;
        while (next < et.size() && et[next].yMin <= sy) {
            const Edge& e = et[next++];
            aet.push_back({e.yMax, e.xAtYMin + (sy - e.yMin) * e.invSlope, e.invSlope});
        }
        aet.erase(std::remove_if(aet.begin(), aet.end(), [&](const Active& a) { return a.yMax <= sy; }), aet.end());
        std::sort(aet.begin(), aet.end(), [](const Active& l, const Active& r) { return l.x < r.x; });

        if (y >= yStart) {
            for (size_t i = 0; i + 1 < aet.size(); i += 2) {
                const int x0 = (int)std::ceil(aet[i].x - 0.5f);
                const int x1 = (int)std::ceil(aet[i + 1].x - 0.5f) - 1;
                if (x1 >= x0) fb.hLine(x0, x1, y, c);
            }
        }
        for (Active& a : aet) a.x += a.invSlope;
    }
}

bool pointInPolygon(const Polygon& p, Vec2 pt) {
    bool inside = false;
    const size_t n = p.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const bool crosses = (p[i].y > pt.y) != (p[j].y > pt.y);
        if (crosses && pt.x < (p[j].x - p[i].x) * (pt.y - p[i].y) / (p[j].y - p[i].y) + p[i].x) inside = !inside;
    }
    return inside;
}

Polygon transformPolygon(const Polygon& p, Vec2 pivot, float angle, float scale, Vec2 translate) {
    Polygon out;
    out.reserve(p.size());
    for (const Vec2& v : p) out.push_back(pivot + rotate((v - pivot) * scale, angle) + translate);
    return out;
}
