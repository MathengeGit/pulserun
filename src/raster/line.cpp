#include "raster/line.h"
#include "raster/circle.h"
#include "raster/polygon.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// DDA (Digital Differential Analyzer): step one pixel along the longer axis and
// add a constant fractional increment on the other axis. Uses floating point.
std::vector<IPoint> ddaPoints(int x0, int y0, int x1, int y1) {
    std::vector<IPoint> pts;
    const int dx = x1 - x0, dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) {
        pts.push_back({x0, y0});
        return pts;
    }
    const float xInc = dx / (float)steps, yInc = dy / (float)steps;
    float x = (float)x0, y = (float)y0;
    for (int i = 0; i <= steps; ++i) {
        pts.push_back({(int)std::lround(x), (int)std::lround(y)});
        x += xInc;
        y += yInc;
    }
    return pts;
}

// Bresenham: integer-only. A decision variable `err` tracks whether the true
// line has drifted far enough to need a step on the minor axis.
// All 8 octants: swap x/y when the line is steep (|dy| > |dx|), swap the end
// points when it runs right-to-left, and flip the y step when it runs upward.
std::vector<IPoint> bresenhamPoints(int x0, int y0, int x1, int y1) {
    std::vector<IPoint> pts;
    const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    bool swapped = false;
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
        swapped = true;
    }
    const int dx = x1 - x0, dy = std::abs(y1 - y0);
    const int yStep = (y0 < y1) ? 1 : -1;
    int err = 2 * dy - dx;
    int y = y0;
    for (int x = x0; x <= x1; ++x) {
        pts.push_back(steep ? IPoint{y, x} : IPoint{x, y});
        if (err > 0) {
            y += yStep;
            err -= 2 * dx;
        }
        err += 2 * dy;
    }
    if (swapped) std::reverse(pts.begin(), pts.end());  // keep start->end order
    return pts;
}

void drawLineDDA(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c) {
    for (const IPoint& p : ddaPoints(x0, y0, x1, y1)) fb.blendPixel(p.x, p.y, c);
}

void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c) {
    for (const IPoint& p : bresenhamPoints(x0, y0, x1, y1)) fb.blendPixel(p.x, p.y, c);
}

void drawDashedLine(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c, int dash, int gap) {
    const auto pts = bresenhamPoints(x0, y0, x1, y1);
    const int period = std::max(1, dash + gap);
    for (size_t i = 0; i < pts.size(); ++i)
        if ((int)(i % period) < dash) fb.blendPixel(pts[i].x, pts[i].y, c);
}

void drawThickLine(Framebuffer& fb, Vec2 a, Vec2 b, float width, Color c, bool roundCaps) {
    const Vec2 d = b - a;
    const float len = length(d);
    if (len < 1e-4f) {
        fillCircle(fb, (int)std::lround(a.x), (int)std::lround(a.y), std::max(1, (int)(width / 2)), c);
        return;
    }
    const Vec2 n(-d.y / len * (width * 0.5f), d.x / len * (width * 0.5f));
    fillPolygon(fb, Polygon{a + n, b + n, b - n, a - n}, c);
    if (roundCaps) {
        const int r = std::max(1, (int)(width * 0.5f));  // floor: disc diameter 2r+1 matches the stroke
        fillCircle(fb, (int)std::lround(a.x), (int)std::lround(a.y), r, c);
        fillCircle(fb, (int)std::lround(b.x), (int)std::lround(b.y), r, c);
    }
}
