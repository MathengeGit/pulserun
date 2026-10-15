#include "raster/circle.h"
#include "math/vec2.h"
#include "raster/polygon.h"
#include <algorithm>
#include <cmath>

namespace {

// Midpoint circle algorithm. Generates the first octant (0 <= x <= y); the other
// seven octants follow by symmetry. Integer arithmetic only.
// Decision variable d = F(x+1, y-1/2) where F = x^2 + y^2 - r^2.
template <class F>
void midpointCircle(int r, F plot) {
    int x = 0, y = r, d = 1 - r;
    plot(x, y);
    while (x < y) {
        ++x;
        if (d < 0) {
            d += 2 * x + 1;  // midpoint inside the circle: keep y
        } else {
            --y;
            d += 2 * (x - y) + 1;  // midpoint outside: step y down
        }
        plot(x, y);
    }
}

// Midpoint ellipse algorithm: two regions split where the curve slope is -1.
template <class F>
void midpointEllipse(int rx, int ry, F plot) {
    const double rx2 = (double)rx * rx, ry2 = (double)ry * ry;
    int x = 0, y = ry;
    double px = 0, py = 2 * rx2 * y;
    double p = ry2 - rx2 * ry + 0.25 * rx2;
    while (px < py) {  // region 1: slope magnitude < 1, step in x
        plot(x, y);
        ++x;
        px += 2 * ry2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            --y;
            py -= 2 * rx2;
            p += ry2 + px - py;
        }
    }
    p = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (double)(y - 1) * (y - 1) - rx2 * ry2;
    while (y >= 0) {  // region 2: slope magnitude >= 1, step in y
        plot(x, y);
        --y;
        py -= 2 * rx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            ++x;
            px += 2 * ry2;
            p += rx2 - py + px;
        }
    }
}

// The 8 symmetric points of (x,y), duplicates removed so alpha never double-blends.
template <class F>
void forEachSymmetric8(int x, int y, F fn) {
    const int xs[8] = {x, -x, x, -x, y, -y, y, -y};
    const int ys[8] = {y, y, -y, -y, x, x, -x, -x};
    for (int i = 0; i < 8; ++i) {
        bool dup = false;
        for (int j = 0; j < i; ++j)
            if (xs[j] == xs[i] && ys[j] == ys[i]) dup = true;
        if (!dup) fn(xs[i], ys[i]);
    }
}

float normDeg(float a) {
    a = std::fmod(a, 360.0f);
    return a < 0 ? a + 360.0f : a;
}

}  // namespace

std::vector<int> circleHalfWidths(int r) {
    std::vector<int> hw((size_t)r + 1, 0);
    midpointCircle(r, [&](int x, int y) {
        hw[y] = std::max(hw[y], x);  // row y reaches out to x
        hw[x] = std::max(hw[x], y);  // symmetric: row x reaches out to y
    });
    return hw;
}

void drawCircle(Framebuffer& fb, int cx, int cy, int r, Color c) {
    if (r <= 0) {
        fb.blendPixel(cx, cy, c);
        return;
    }
    midpointCircle(r, [&](int x, int y) {
        forEachSymmetric8(x, y, [&](int dx, int dy) { fb.blendPixel(cx + dx, cy + dy, c); });
    });
}

void drawCircleThick(Framebuffer& fb, int cx, int cy, int r, int thickness, Color c) {
    for (int k = 0; k < thickness; ++k) drawCircle(fb, cx, cy, r - thickness / 2 + k, c);
}

void fillCircle(Framebuffer& fb, int cx, int cy, int r, Color c) {
    if (r <= 0) {
        fb.blendPixel(cx, cy, c);
        return;
    }
    const std::vector<int> hw = circleHalfWidths(r);
    for (int dy = 0; dy <= r; ++dy) {
        fb.hLine(cx - hw[dy], cx + hw[dy], cy + dy, c);
        if (dy > 0) fb.hLine(cx - hw[dy], cx + hw[dy], cy - dy, c);
    }
}

void drawArc(Framebuffer& fb, int cx, int cy, int r, float startDeg, float endDeg, Color c) {
    if (r <= 0) return;
    const bool full = (endDeg - startDeg) >= 360.0f;
    const float s = normDeg(startDeg) - 0.75f, e = normDeg(endDeg) + 0.75f;
    auto inArc = [&](float a) {
        if (full) return true;
        a = normDeg(a);
        const float s2 = normDeg(s), e2 = normDeg(e);
        return s2 <= e2 ? (a >= s2 && a <= e2) : (a >= s2 || a <= e2);
    };
    midpointCircle(r, [&](int x, int y) {
        forEachSymmetric8(x, y, [&](int dx, int dy) {
            const float ang = std::atan2((float)(-dy), (float)dx) * 180.0f / kPi;  // y is down
            if (inArc(ang)) fb.blendPixel(cx + dx, cy + dy, c);
        });
    });
}

// Thick arcs are a filled band: points along the outer radius, then back along the
// inner radius, filled with the scanline polygon filler. Concentric 1px midpoint
// circles would leave small gaps on the diagonals, so this looks cleaner.
void drawArcThick(Framebuffer& fb, int cx, int cy, int r, float startDeg, float endDeg, int thickness, Color c) {
    if (thickness <= 1) {
        drawArc(fb, cx, cy, r, startDeg, endDeg, c);
        return;
    }
    float sweep = endDeg - startDeg;
    if (sweep <= 0) sweep += 360.0f;
    sweep = std::min(sweep, 360.0f);
    const float rOut = r + thickness * 0.5f, rIn = std::max(0.5f, r - thickness * 0.5f);
    const int n = std::max(8, (int)std::ceil(sweep / 2.0f));
    const float ox = cx + 0.5f, oy = cy + 0.5f;  // pixel-centre alignment with the midpoint circle
    Polygon band;
    for (int k = 0; k <= n; ++k) {
        const float a = (startDeg + sweep * k / n) * kPi / 180.0f;
        band.push_back(Vec2(ox + rOut * std::cos(a), oy - rOut * std::sin(a)));
    }
    for (int k = n; k >= 0; --k) {
        const float a = (startDeg + sweep * k / n) * kPi / 180.0f;
        band.push_back(Vec2(ox + rIn * std::cos(a), oy - rIn * std::sin(a)));
    }
    fillPolygon(fb, band, c);
}

void drawEllipse(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c) {
    if (rx <= 0 || ry <= 0) {
        fb.hLine(cx - rx, cx + rx, cy, c);
        return;
    }
    midpointEllipse(rx, ry, [&](int x, int y) {
        fb.blendPixel(cx + x, cy + y, c);
        if (x != 0) fb.blendPixel(cx - x, cy + y, c);
        if (y != 0) fb.blendPixel(cx + x, cy - y, c);
        if (x != 0 && y != 0) fb.blendPixel(cx - x, cy - y, c);
    });
}

void fillEllipse(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c) {
    if (rx <= 0 || ry <= 0) {
        fb.hLine(cx - rx, cx + rx, cy, c);
        return;
    }
    std::vector<int> hw((size_t)ry + 1, 0);
    midpointEllipse(rx, ry, [&](int x, int y) { hw[y] = std::max(hw[y], x); });
    for (int dy = 0; dy <= ry; ++dy) {
        fb.hLine(cx - hw[dy], cx + hw[dy], cy + dy, c);
        if (dy > 0) fb.hLine(cx - hw[dy], cx + hw[dy], cy - dy, c);
    }
}
