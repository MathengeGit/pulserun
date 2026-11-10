// Numeric checks for the graphics algorithms. No test framework needed:
//   cmake --build build --target test_algorithms && ./build/test_algorithms
#include "game/camera.h"
#include "game/game_loop.h"
#include "game/player.h"
#include "math/mat3.h"
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

static bool near(Vec2 a, Vec2 b, float eps = 1e-4f) { return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps; }

static void testMat3() {
    // Identity leaves points unchanged.
    CHECK(near(Mat3::identity() * Vec2(3.5f, -7), Vec2(3.5f, -7)));
    // Translation moves the origin.
    CHECK(near(Mat3::translate(5, -3) * Vec2(0, 0), Vec2(5, -3)));
    // Quarter turn maps +x to +y (clockwise on a y-down screen).
    CHECK(near(Mat3::rotate(kPi / 2) * Vec2(1, 0), Vec2(0, 1)));
    // Scale is per axis.
    CHECK(near(Mat3::scale(2, 3) * Vec2(4, 5), Vec2(8, 15)));
    // Chained T*R*S applies scale, then rotate, then translate: (1,0) -> (2,0) -> (0,2) -> (10,22).
    const Mat3 trs = Mat3::translate(10, 20) * Mat3::rotate(kPi / 2) * Mat3::scale(2, 2);
    CHECK(near(trs * Vec2(1, 0), Vec2(10, 22)));
    // Order matters: S*R*T is a different transform.
    CHECK(!near(Mat3::scale(2, 2) * Mat3::rotate(kPi / 2) * Mat3::translate(10, 20) * Vec2(1, 0), Vec2(10, 22)));
    // m * m.inverse() is the identity, and inverse undoes the transform.
    const Mat3 m = Mat3::translate(-40, 13) * Mat3::rotate(0.7f) * Mat3::scale(1.5f, 0.5f);
    const Mat3 id = m * m.inverse();
    bool isId = true;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            if (std::fabs(id.at(r, c) - (r == c ? 1.0f : 0.0f)) > 1e-5f) isId = false;
    CHECK(isId);
    CHECK(near(m.inverse() * (m * Vec2(123, -45)), Vec2(123, -45), 1e-3f));
    CHECK(std::fabs(Mat3::scale(2, 3).determinant() - 6) < 1e-6f);
    // Directions ignore translation.
    CHECK(near(Mat3::translate(100, 100).applyVector(Vec2(1, 2)), Vec2(1, 2)));
    // Rotating about a pivot keeps the pivot fixed.
    CHECK(near(Mat3::rotateAbout(Vec2(50, 50), 1.1f) * Vec2(50, 50), Vec2(50, 50)));
    // Sprite vertex: the cube's top-left corner (size 56 at (200,300)) rotated 90 degrees about
    // its centre lands at the top-right corner, by hand: (172,272) -> (228,272).
    CHECK(near(Mat3::rotateAbout(Vec2(200, 300), kPi / 2) * Vec2(172, 272), Vec2(228, 272), 1e-3f));
    // Mat3 reproduces transformPolygon(): scale+rotate about pivot, then translate.
    const Polygon spike = {Vec2(0, 0), Vec2(24, -48), Vec2(48, 0)};
    const Vec2 pivot(24, -16), offs(300, 440);
    const Mat3 xf = Mat3::translate(offs.x, offs.y) * Mat3::translate(pivot.x, pivot.y) * Mat3::rotate(0.4f) *
                    Mat3::scale(1.25f, 1.25f) * Mat3::translate(-pivot.x, -pivot.y);
    const Polygon ref = transformPolygon(spike, pivot, 0.4f, 1.25f, offs);
    bool same = true;
    for (size_t i = 0; i < spike.size(); ++i)
        if (!near(xf * spike[i], ref[i], 1e-3f)) same = false;
    CHECK(same);
}

namespace {
struct CountingState : GameState {
    int updates = 0, jumpEdges = 0;
    float lastDt = 0;
    void update(float dt, const Input& in) override {
        ++updates;
        lastDt = dt;
        if (in.jumpPressed) ++jumpEdges;
    }
    void render(Framebuffer&, float) override {}
};
}  // namespace

static void testGameLoop() {
    GameLoop loop(8, 8);
    CountingState s;
    // 50 ms of wall time = 3 whole 60 Hz steps, with the remainder carried as alpha.
    CHECK(loop.advance(0.050, s, Input{}) == 3);
    CHECK(std::fabs(s.lastDt - 1.0f / 60.0f) < 1e-7f);
    CHECK(loop.alpha() > 0.0f && loop.alpha() < 1.0f);
    // A jump edge that lands between steps is held until the next step, then seen once.
    Input in;
    in.jumpPressed = true;
    CHECK(loop.advance(0.001, s, in) == 0);
    CHECK(s.jumpEdges == 0);
    loop.advance(0.040, s, Input{});
    CHECK(s.jumpEdges == 1);
    // A long stall is clamped to MAX_FRAME (no spiral of death): at most 15 steps.
    GameLoop l2(8, 8);
    CountingState s2;
    CHECK(l2.advance(10.0, s2, Input{}) == (int)(GameLoop::MAX_FRAME / GameLoop::FIXED_DT + 1e-6));
    // Splitting the same wall time into odd frame lengths gives the same step count.
    GameLoop l3(8, 8);
    CountingState s3;
    for (int i = 0; i < 70; ++i) l3.advance(i % 2 ? 0.007 : 0.0213, s3, Input{});
    CHECK(s3.updates == (int)((35 * 0.007 + 35 * 0.0213) * 60));
}

static void testCamera() {
    Camera cam;
    cam.scroll_x = 1000;
    // world_to_screen() is translate(-scroll_x, 0): world x 1260 appears at screen x 260.
    CHECK(near(cam.toScreen(Vec2(1260, 300)), Vec2(260, 300)));
    // screen_to_world() is its exact inverse.
    CHECK(near(cam.screen_to_world() * Vec2(260, 300), Vec2(1260, 300), 1e-3f));
    // reset() keeps the player 260 px from the left and puts world ground y=500 at screen y=440.
    cam.reset(Vec2(4000, 470), 500);
    CHECK(near(cam.toScreen(Vec2(4000, 500)), Vec2(260, 440), 1e-3f));
    // follow() scrolls horizontally with the player every step.
    cam.follow(Vec2(4100, 470), 500, 1.0f / 60);
    CHECK(std::fabs(cam.toScreen(Vec2(4100, 0)).x - 260) < 1e-3f);
    // A player high above the ground pulls the camera up (scroll_y decreases), but never past it.
    Camera up;
    up.reset(Vec2(0, 470), 500);
    for (int i = 0; i < 120; ++i) up.follow(Vec2(0, 40), 500, 1.0f / 60);
    CHECK(up.scroll_y < 60 - 100 && up.toScreen(Vec2(0, 40)).y >= 149.0f);
    // Shake offsets the screen without moving the world-space scroll.
    Camera sh = cam;
    sh.shake_x = 5;
    CHECK(near(sh.toScreen(Vec2(4100, 0)) - cam.toScreen(Vec2(4100, 0)), Vec2(5, 0)));
}

// Simulate one jump on flat ground (y = 500) holding the jump for `holdSteps`; returns the apex height.
static float jumpApex(int holdSteps) {
    Player p;
    p.reset(Vec2(0, 500 - Player::SIZE / 2), 0);
    p.setHeld(holdSteps > 0);
    p.jump();
    float top = p.bottom();
    for (int i = 0; i < 240; ++i) {
        p.setHeld(i < holdSteps);
        p.update(1.0f / 60);
        top = std::min(top, p.bottom());
        if (p.bottom() >= 500) { p.land(500); break; }
    }
    return 500 - top;
}

static void testPhysics() {
    const float dt = 1.0f / 60;
    // Gravity integration: 0.5 s of free fall from rest is close to g*t^2/2 = 150 px
    // (semi-implicit Euler runs slightly ahead: 155 px) and the speed is exactly g*t.
    Player p;
    p.reset(Vec2(0, 0), 0);
    p.on_ground = false;
    for (int i = 0; i < 30; ++i) p.update(dt);
    CHECK(std::fabs(p.pos.y - 155.0f) < 0.5f);
    CHECK(std::fabs(p.vel.y - 600.0f) < 1e-2f);
    // Terminal velocity caps the fall speed.
    for (int i = 0; i < 120; ++i) p.update(dt);
    CHECK(p.vel.y == Player::TERMINAL_VEL);
    // Jump only from the ground; it sets vel.y = -600.
    CHECK(!p.jump());
    p.land(500);
    CHECK(p.on_ground && p.bottom() == 500 && p.vel.y == 0);
    CHECK(p.jump() && p.vel.y == -Player::JUMP_VEL && !p.on_ground);
    // Horizontal run speed is constant.
    Player r;
    r.reset(Vec2(0, 0), 420);
    for (int i = 0; i < 60; ++i) r.update(dt);
    CHECK(std::fabs(r.pos.x - 420) < 1e-2f);
    // A tap gives v^2 / 2g = 150 px (slightly less with Euler steps).
    const float tap = jumpApex(0);
    CHECK(tap > 140 && tap < 151);
    // Holding raises the jump, and the extension is capped at 350 ms: holding for 2 s is the
    // same as holding for exactly 21 steps (0.35 s).
    const float held = jumpApex(120);
    CHECK(held > tap + 35);
    CHECK(std::fabs(held - jumpApex(21)) < 0.5f);
    CHECK(jumpApex(10) > tap && jumpApex(10) < held);
    // The cube spins in the air and settles on a quarter turn after landing.
    Player s;
    s.reset(Vec2(0, 500 - Player::SIZE / 2), 0);
    s.jump();
    for (int i = 0; i < 20; ++i) s.update(dt);
    CHECK(s.angle > 1.0f);
    s.land(500);
    for (int i = 0; i < 30; ++i) {
        s.update(dt);
        s.land(500);
    }
    const float q = s.angle / (kPi / 2);
    CHECK(std::fabs(q - std::round(q)) < 1e-4f);
}

int main() {
    testBresenham();
    testCircleAndEllipse();
    testPolygon();
    testClipping();
    testCurves();
    testMat3();
    testGameLoop();
    testCamera();
    testPhysics();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
