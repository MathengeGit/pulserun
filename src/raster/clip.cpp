#include "raster/clip.h"
#include "raster/line.h"
#include <cmath>

namespace {
// Region codes: one bit per side of the window (y grows downward).
constexpr int kLeft = 1, kRight = 2, kBottom = 4, kTop = 8;

int outcode(double x, double y, const ClipRect& r) {
    int code = 0;
    if (x < r.xmin) code |= kLeft;
    else if (x > r.xmax) code |= kRight;
    if (y < r.ymin) code |= kTop;
    else if (y > r.ymax) code |= kBottom;
    return code;
}
}  // namespace

// Cohen-Sutherland: classify both end points with outcodes.
//  - both inside (code 0|0)       -> trivially accept
//  - codes share a bit (code & code) -> both on the outer side of one edge: trivially reject
//  - otherwise move an outside end point onto the window edge it violates and repeat.
bool clipLineCohenSutherland(double& x0, double& y0, double& x1, double& y1, const ClipRect& r) {
    int c0 = outcode(x0, y0, r), c1 = outcode(x1, y1, r);
    while (true) {
        if (!(c0 | c1)) return true;
        if (c0 & c1) return false;
        const int out = c0 ? c0 : c1;
        double x = 0, y = 0;
        if (out & kTop) {
            x = x0 + (x1 - x0) * (r.ymin - y0) / (y1 - y0);
            y = r.ymin;
        } else if (out & kBottom) {
            x = x0 + (x1 - x0) * (r.ymax - y0) / (y1 - y0);
            y = r.ymax;
        } else if (out & kRight) {
            y = y0 + (y1 - y0) * (r.xmax - x0) / (x1 - x0);
            x = r.xmax;
        } else {
            y = y0 + (y1 - y0) * (r.xmin - x0) / (x1 - x0);
            x = r.xmin;
        }
        if (out == c0) {
            x0 = x;
            y0 = y;
            c0 = outcode(x0, y0, r);
        } else {
            x1 = x;
            y1 = y;
            c1 = outcode(x1, y1, r);
        }
    }
}

// Sutherland-Hodgman: clip the polygon against one window edge at a time. For
// each polygon edge (prev -> cur) there are four cases:
//   in->in   output cur            out->in  output intersection, cur
//   in->out  output intersection   out->out output nothing
Polygon clipPolygonSutherlandHodgman(const Polygon& in, const ClipRect& r) {
    Polygon out = in;
    auto clipAgainst = [&](auto inside, auto intersect) {
        if (out.empty()) return;
        const Polygon input = out;
        out.clear();
        Vec2 prev = input.back();
        for (const Vec2& cur : input) {
            const bool curIn = inside(cur), prevIn = inside(prev);
            if (curIn) {
                if (!prevIn) out.push_back(intersect(prev, cur));
                out.push_back(cur);
            } else if (prevIn) {
                out.push_back(intersect(prev, cur));
            }
            prev = cur;
        }
    };
    const float xmin = (float)r.xmin, xmax = (float)r.xmax, ymin = (float)r.ymin, ymax = (float)r.ymax;
    clipAgainst([&](Vec2 p) { return p.x >= xmin; },
                [&](Vec2 a, Vec2 b) { return Vec2(xmin, a.y + (xmin - a.x) / (b.x - a.x) * (b.y - a.y)); });
    clipAgainst([&](Vec2 p) { return p.x <= xmax; },
                [&](Vec2 a, Vec2 b) { return Vec2(xmax, a.y + (xmax - a.x) / (b.x - a.x) * (b.y - a.y)); });
    clipAgainst([&](Vec2 p) { return p.y >= ymin; },
                [&](Vec2 a, Vec2 b) { return Vec2(a.x + (ymin - a.y) / (b.y - a.y) * (b.x - a.x), ymin); });
    clipAgainst([&](Vec2 p) { return p.y <= ymax; },
                [&](Vec2 a, Vec2 b) { return Vec2(a.x + (ymax - a.y) / (b.y - a.y) * (b.x - a.x), ymax); });
    return out;
}

void drawLineClipped(Framebuffer& fb, double x0, double y0, double x1, double y1, const ClipRect& r, Color c) {
    if (clipLineCohenSutherland(x0, y0, x1, y1, r))
        drawLineBresenham(fb, (int)std::lround(x0), (int)std::lround(y0), (int)std::lround(x1),
                          (int)std::lround(y1), c);
}

void fillPolygonClipped(Framebuffer& fb, const Polygon& p, const ClipRect& r, Color c) {
    const Polygon clipped = clipPolygonSutherlandHodgman(p, r);
    if (clipped.size() >= 3) fillPolygon(fb, clipped, c);
}
