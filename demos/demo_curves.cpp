// Demo 5: curves - cubic Bezier (De Casteljau) and Catmull-Rom splines
#include "common.h"
#include "raster/circle.h"
#include "raster/curve.h"
#include "raster/line.h"
#include "ui/ui.h"
#include <cmath>

static void dot(Framebuffer& fb, Vec2 p, int r, Color c) { fillCircle(fb, (int)std::lround(p.x), (int)std::lround(p.y), r, c); }

static void dashedSegment(Framebuffer& fb, Vec2 a, Vec2 b, Color c) {
    drawDashedLine(fb, (int)std::lround(a.x), (int)std::lround(a.y), (int)std::lround(b.x), (int)std::lround(b.y), c, 5, 4);
}

int main(int argc, char** argv) {
    Framebuffer fb(960, 540);
    fb.clear(demoBg());
    const Palette pal = defaultPalette();
    drawText(fb, 24, 18, "BEZIER CURVES AND SPLINES", 4, demoText());

    // ---- Panel A: cubic Bezier with the De Casteljau construction
    drawRoundedRect(fb, 20, 70, 440, 260, 14, demoFrame(), 2);
    drawText(fb, 36, 84, "BEZIER + DE CASTELJAU (T=0.4)", 2, demoDim());
    const Vec2 p0(60, 290), p1(100, 120), p2(380, 110), p3(420, 285);
    dashedSegment(fb, p0, p1, demoDim());
    dashedSegment(fb, p1, p2, demoDim());
    dashedSegment(fb, p2, p3, demoDim());
    drawBezier(fb, p0, p1, p2, p3, 5.0f, pal.neon);
    const auto lv = deCasteljauLevels(p0, p1, p2, p3, 0.4f);
    for (size_t i = 0; i + 1 < lv[0].size(); ++i) drawThickLine(fb, lv[0][i], lv[0][i + 1], 2.0f, pal.neon2);
    drawThickLine(fb, lv[1][0], lv[1][1], 2.0f, pal.accent);
    for (const Vec2& p : lv[0]) dot(fb, p, 4, pal.neon2);
    for (const Vec2& p : lv[1]) dot(fb, p, 4, pal.accent);
    dot(fb, lv[2][0], 7, rgb(255, 255, 255));
    for (const Vec2& p : {p0, p1, p2, p3}) dot(fb, p, 6, rgb(255, 255, 255));
    drawText(fb, 44, 262, "P0", 2, demoText());
    drawText(fb, 84, 98 + 28, "P1", 2, demoText());
    drawText(fb, 372, 126 + 4, "P2", 2, demoText());
    drawText(fb, 422, 262, "P3", 2, demoText());

    // ---- Panel B: Catmull-Rom spline through points
    drawRoundedRect(fb, 480, 70, 460, 260, 14, demoFrame(), 2);
    drawText(fb, 496, 84, "CATMULL-ROM SPLINE", 2, demoDim());
    const std::vector<Vec2> pts = {Vec2(510, 280), Vec2(580, 160), Vec2(660, 270), Vec2(740, 140), Vec2(820, 255), Vec2(910, 150)};
    for (size_t i = 0; i + 1 < pts.size(); ++i) dashedSegment(fb, pts[i], pts[i + 1], demoDim());
    drawPolylineThick(fb, sampleCatmullRom(pts, 24), 5.0f, pal.neon2);
    for (const Vec2& p : pts) dot(fb, p, 6, rgb(255, 255, 255));

    // ---- Panel C: game application - jump arc, trail, cube, spike, portal
    drawRoundedRect(fb, 20, 345, 920, 185, 14, demoFrame(), 2);
    drawText(fb, 36, 359, "APPLIED: JUMP ARC TRAIL (BEZIER) + PORTAL (2 BEZIERS)", 2, demoDim());
    const int groundY = 505;
    for (int y = groundY; y < 528; ++y) fb.hLine(30, 930, y, lerpColor(pal.groundTop, pal.groundBottom, (y - groundY) / 23.0f));
    fb.fillRect(30, groundY, 900, 3, pal.neon);
    const Vec2 a0(80, 485), a1(230, 385), a2(470, 385), a3(620, 485);
    drawSpike(fb, 350, (float)groundY, 40, 40, pal);
    drawJumpTrail(fb, a0, a1, a2, a3, 0.58f, pal.cube);
    drawCube(fb, bezierPoint(a0, a1, a2, a3, 0.58f), 38, 0.58f * kPi / 2, pal);
    drawPortal(fb, Vec2(800, 452), 22, 50, pal.neon2);
    drawOrb(fb, Vec2(690, 440), 14, 0.5f, pal);

    return saveDemo(fb, argc, argv, "05_curves.png");
}
