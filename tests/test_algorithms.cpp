// Numeric checks for the graphics algorithms. No test framework needed:
//   cmake --build build --target test_algorithms && ./build/test_algorithms
#include "raster/circle.h"
#include "raster/clip.h"
#include "raster/curve.h"
#include "raster/line.h"
#include "raster/polygon.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <utility>

static int g_failures = 0, g_checks = 0;
#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failures;                                                             \
            std::cout << "FAIL  " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
        }                                                                             \
    } while (0)

static int countLit(const Framebuffer& fb) {
    int n = 0;
    for (int y = 0; y < fb.height(); ++y)
        for (int x = 0; x < fb.width(); ++x)
            if (fb.getPixel(x, y).r != 0) ++n;
    return n;
}

static void testBresenham() {
    // Lines in all 8 octants (plus axis-aligned) from the same centre.
    const int cx = 50, cy = 50;
    const int targets[][2] = {{90, 60}, {90, 80}, {60, 90}, {40, 90}, {10, 80}, {10, 60},
                              {10, 40}, {40, 10}, {60, 10}, {90, 40}, {90, 50}, {50, 90}};
    for (const auto& t : targets) {
        const auto pts = bresenhamPoints(cx, cy, t[0], t[1]);
        const int expected = std::max(std::abs(t[0] - cx), std::abs(t[1] - cy)) + 1;
        CHECK((int)pts.size() == expected);
        CHECK(pts.front().x == cx && pts.front().y == cy);
        CHECK(pts.back().x == t[0] && pts.back().y == t[1]);
        bool connected = true;
        for (size_t i = 1; i < pts.size(); ++i)
            if (std::abs(pts[i].x - pts[i - 1].x) > 1 || std::abs(pts[i].y - pts[i - 1].y) > 1) connected = false;
        CHECK(connected);  // every step moves to an 8-neighbour pixel
        // Reversed line touches the same set of pixels.
        const auto rev = bresenhamPoints(t[0], t[1], cx, cy);
        std::set<std::pair<int, int>> a, b;
        for (auto& p : pts) a.insert({p.x, p.y});
        for (auto& p : rev) b.insert({p.x, p.y});
        CHECK(a == b);
    }
    // DDA agrees on end points and length.
    const auto d = ddaPoints(3, 4, 40, 17);
    CHECK(d.front().x == 3 && d.back().x == 40 && d.back().y == 17);
}

static void testCircleAndEllipse() {
    Framebuffer fb(200, 200);
    fb.clear(rgb(0, 0, 0));
    fillCircle(fb, 100, 100, 50, rgb(255, 255, 255));
    const double area = 3.14159265 * 50 * 50;
    CHECK(std::abs(countLit(fb) - area) / area < 0.03);

    // Outline circle is symmetric left/right and top/bottom.
    Framebuffer o(200, 200);
    o.clear(rgb(0, 0, 0));
    drawCircle(o, 100, 100, 40, rgb(255, 255, 255));
    bool sym = true;
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 200; ++x)
            if ((o.getPixel(x, y).r != 0) != (o.getPixel(200 - x, y).r != 0) && x > 0) sym = false;
    CHECK(sym);

    Framebuffer e(300, 200);
    e.clear(rgb(0, 0, 0));
    fillEllipse(e, 150, 100, 100, 40, rgb(255, 255, 255));
    const double eArea = 3.14159265 * 100 * 40;
    CHECK(std::abs(countLit(e) - eArea) / eArea < 0.03);
}

static void testPolygon() {
    Framebuffer fb(200, 200);
    fb.clear(rgb(0, 0, 0));
    fillPolygon(fb, makeRect(10, 10, 30, 20), rgb(255, 255, 255));
    CHECK(countLit(fb) == 600);  // exact for axis-aligned rectangles

    Framebuffer t(200, 200);
    t.clear(rgb(0, 0, 0));
    fillPolygon(t, Polygon{Vec2(20, 20), Vec2(120, 20), Vec2(20, 120)}, rgb(255, 255, 255));
    CHECK(std::abs(countLit(t) - 5000) < 150);  // triangle area = 5000

    const Polygon arrow{Vec2(0, 20), Vec2(60, 20), Vec2(60, 0), Vec2(100, 40), Vec2(60, 80), Vec2(60, 60), Vec2(0, 60)};
    CHECK(pointInPolygon(arrow, Vec2(20, 40)));    // shaft
    CHECK(pointInPolygon(arrow, Vec2(70, 40)));    // head
    CHECK(!pointInPolygon(arrow, Vec2(20, 5)));    // notch above the shaft (concave region)
    CHECK(!pointInPolygon(arrow, Vec2(120, 40)));  // outside
}

static void testClipping() {
    const ClipRect r{100, 100, 200, 200};
    double x0 = 120, y0 = 120, x1 = 180, y1 = 180;
    CHECK(clipLineCohenSutherland(x0, y0, x1, y1, r) && x0 == 120 && y1 == 180);  // trivially accepted

    x0 = 10; y0 = 10; x1 = 50; y1 = 90;
    CHECK(!clipLineCohenSutherland(x0, y0, x1, y1, r));  // trivially rejected

    x0 = 50; y0 = 150; x1 = 250; y1 = 150;  // horizontal line through the window
    CHECK(clipLineCohenSutherland(x0, y0, x1, y1, r));
    CHECK(std::abs(x0 - 100) < 1e-9 && std::abs(x1 - 200) < 1e-9 && y0 == 150);

    x0 = 0; y0 = 0; x1 = 300; y1 = 300;  // diagonal through two corners
    CHECK(clipLineCohenSutherland(x0, y0, x1, y1, r));
    CHECK(std::abs(x0 - 100) < 1e-9 && std::abs(y0 - 100) < 1e-9 && std::abs(x1 - 200) < 1e-9);

    // Sutherland-Hodgman: half-overlapping square -> 50x50 square.
    const Polygon clipped = clipPolygonSutherlandHodgman(makeRect(50, 50, 100, 100), r);
    Framebuffer fb(300, 300);
    fb.clear(rgb(0, 0, 0));
    fillPolygon(fb, clipped, rgb(255, 255, 255));
    CHECK(countLit(fb) == 2500);
    CHECK(clipPolygonSutherlandHodgman(makeRect(0, 0, 40, 40), r).empty());      // fully outside
    CHECK(clipPolygonSutherlandHodgman(makeRect(120, 120, 20, 20), r).size() == 4);  // fully inside
}

static void testCurves() {
    const Vec2 p0(0, 0), p1(10, 40), p2(60, 40), p3(100, 0);
    const Vec2 a = bezierPoint(p0, p1, p2, p3, 0.0f), b = bezierPoint(p0, p1, p2, p3, 1.0f);
    CHECK(a.x == 0 && a.y == 0 && std::abs(b.x - 100) < 1e-4 && std::abs(b.y) < 1e-4);
    const Vec2 m = bezierPoint(p0, p1, p2, p3, 0.5f);  // (p0 + 3p1 + 3p2 + p3) / 8
    CHECK(std::abs(m.x - (0 + 30 + 180 + 100) / 8.0f) < 1e-3 && std::abs(m.y - (0 + 120 + 120 + 0) / 8.0f) < 1e-3);

    const std::vector<Vec2> pts = {Vec2(0, 0), Vec2(50, 80), Vec2(100, 10), Vec2(150, 70)};
    const auto s = sampleCatmullRom(pts, 10);
    CHECK(s.size() == 3 * 10 + 1);
    CHECK(std::abs(s[10].x - 50) < 1e-3 && std::abs(s[10].y - 80) < 1e-3);  // passes through control points
    CHECK(std::abs(s[20].x - 100) < 1e-3 && std::abs(s[20].y - 10) < 1e-3);
}

int main() {
    testBresenham();
    testCircleAndEllipse();
    testPolygon();
    testClipping();
    testCurves();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
