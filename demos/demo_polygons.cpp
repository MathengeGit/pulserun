// Demo 3: polygons - scanline fill, point-in-polygon, 2D transforms
#include "common.h"
#include "raster/circle.h"
#include "raster/line.h"
#include "raster/polygon.h"
#include "ui/ui.h"
#include <cmath>

static void pipGrid(Framebuffer& fb, const Polygon& poly, int x0, int y0, int x1, int y1, int step) {
    for (int y = y0; y <= y1; y += step)
        for (int x = x0; x <= x1; x += step) {
            const bool in = pointInPolygon(poly, Vec2((float)x, (float)y));
            fillCircle(fb, x, y, in ? 3 : 1, in ? rgb(90, 255, 120) : rgb(255, 80, 100));
        }
}

int main(int argc, char** argv) {
    Framebuffer fb(960, 540);
    fb.clear(demoBg());
    const Palette pal = defaultPalette();

    drawText(fb, 24, 18, "POLYGON FILL AND TRANSFORMS", 4, demoText());

    // ---- Row 1: filled regular polygons (scanline fill + outline)
    drawRoundedRect(fb, 20, 70, 920, 170, 14, demoFrame(), 2);
    drawText(fb, 36, 84, "SCANLINE FILL: 3 / 4 / 5 / 6 / 8 SIDES", 2, demoDim());
    const int sides[5] = {3, 4, 5, 6, 8};
    for (int i = 0; i < 5; ++i) {
        const Polygon p = regularPolygon(Vec2(110.0f + i * 175.0f, 165), 58, sides[i], i == 1 ? kPi / 4 : 0);
        fillPolygon(fb, p, withAlpha(lerpColor(pal.neon, pal.neon2, i / 4.0f), 200));
        drawPolygonThick(fb, p, 3.0f, rgb(240, 245, 255), true);
    }

    // ---- Row 2 left: concave polygons + point-in-polygon sampling
    drawRoundedRect(fb, 20, 255, 460, 265, 14, demoFrame(), 2);
    drawText(fb, 36, 269, "CONCAVE FILL + POINT-IN-POLYGON", 2, demoDim());
    const Polygon star = starPolygon(Vec2(140, 405), 105, 42, 5);
    fillPolygon(fb, star, withAlpha(pal.neon, 90));
    drawPolygon(fb, star, pal.neon);
    pipGrid(fb, star, 40, 300, 240, 505, 13);
    const Polygon arrow{Vec2(300, 420), Vec2(360, 420), Vec2(360, 345), Vec2(440, 410), Vec2(360, 490), Vec2(360, 450), Vec2(300, 450)};
    fillPolygon(fb, arrow, withAlpha(pal.neon2, 90));
    drawPolygon(fb, arrow, pal.neon2);
    pipGrid(fb, arrow, 290, 330, 460, 500, 13);

    // ---- Row 2 right: 2D transformations (rotate + scale + translate)
    drawRoundedRect(fb, 500, 255, 440, 265, 14, demoFrame(), 2);
    drawText(fb, 516, 269, "TRANSFORMS: ROTATE + SCALE", 2, demoDim());
    const Vec2 ringC(720, 401);
    const Polygon spike{Vec2(-14, 0), Vec2(0, -34), Vec2(14, 0)};  // local-space spike
    for (int i = 0; i < 10; ++i) {
        const float a = 2.0f * kPi * i / 10;
        const Vec2 pos = ringC + Vec2(std::sin(a), -std::cos(a)) * 76.0f;
        const Polygon s = transformPolygon(spike, Vec2(0, 0), a, 1.0f, pos);
        fillPolygon(fb, s, withAlpha(pal.hazard, 110));
        drawPolygonThick(fb, s, 3.0f, pal.hazard, true);
    }
    for (int i = 0; i < 6; ++i) {
        const Polygon sq = makeRect(-34, -34, 68, 68);
        const Polygon t = transformPolygon(sq, Vec2(0, 0), i * 0.22f, 1.0f - i * 0.15f, ringC);
        drawPolygonThick(fb, t, 2.0f, lerpColor(pal.neon, pal.accent, i / 5.0f), true);
    }

    return saveDemo(fb, argc, argv, "03_polygons.png");
}
