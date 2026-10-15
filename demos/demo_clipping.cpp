// Demo 4: clipping - Cohen-Sutherland (lines) and Sutherland-Hodgman (polygons)
#include "common.h"
#include "raster/circle.h"
#include "raster/clip.h"
#include "raster/line.h"
#include "raster/polygon.h"
#include "ui/ui.h"
#include <cstdio>

int main(int argc, char** argv) {
    Framebuffer fb(960, 540);
    fb.clear(demoBg());
    const Palette pal = defaultPalette();
    drawText(fb, 24, 18, "CLIPPING", 4, demoText());

    // ---- Left: Cohen-Sutherland
    drawRoundedRect(fb, 20, 70, 450, 450, 14, demoFrame(), 2);
    drawText(fb, 36, 84, "COHEN-SUTHERLAND LINES", 2, demoDim());
    const ClipRect win1{130, 150, 360, 410};
    drawPolygon(fb, makeRect(130, 150, 230, 260), rgb(255, 255, 255));

    // All end points stay inside the panel (x 35..455, y 115..470).
    const double L[][4] = {{45, 125, 440, 455},  {40, 330, 450, 200}, {160, 200, 330, 380}, {40, 455, 250, 120},
                           {300, 115, 300, 465}, {70, 230, 220, 260}, {420, 160, 455, 380}, {40, 140, 110, 118},
                           {260, 125, 450, 330}, {120, 465, 455, 440}, {150, 190, 200, 395}, {50, 465, 445, 115},
                           {45, 260, 120, 255},  {200, 120, 240, 155}};
    int accepted = 0, rejected = 0, clipped = 0;
    for (const auto& l : L) {
        drawDashedLine(fb, (int)l[0], (int)l[1], (int)l[2], (int)l[3], rgb(90, 98, 150), 4, 4);
        double x0 = l[0], y0 = l[1], x1 = l[2], y1 = l[3];
        const bool inside0 = x0 >= win1.xmin && x0 <= win1.xmax && y0 >= win1.ymin && y0 <= win1.ymax;
        const bool inside1 = x1 >= win1.xmin && x1 <= win1.xmax && y1 >= win1.ymin && y1 <= win1.ymax;
        if (!clipLineCohenSutherland(x0, y0, x1, y1, win1)) {
            ++rejected;
            continue;
        }
        (inside0 && inside1) ? ++accepted : ++clipped;
        drawThickLine(fb, Vec2((float)x0, (float)y0), Vec2((float)x1, (float)y1), 3.0f, pal.cube, true);
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "ACCEPTED %d  CLIPPED %d  REJECTED %d", accepted, clipped, rejected);
    drawText(fb, 36, 500, buf, 2, demoText());

    // ---- Right: Sutherland-Hodgman
    drawRoundedRect(fb, 490, 70, 450, 450, 14, demoFrame(), 2);
    drawText(fb, 506, 84, "SUTHERLAND-HODGMAN POLYGONS", 2, demoDim());
    const ClipRect win2{585, 165, 845, 425};
    const Polygon star = starPolygon(Vec2(715, 295), 185, 85, 5, 0.2f);
    const Polygon tri{Vec2(810, 350), Vec2(925, 400), Vec2(790, 470)};
    for (const Polygon* p : {&star, &tri}) {
        drawPolygon(fb, *p, rgb(90, 98, 150));
        const Polygon c = clipPolygonSutherlandHodgman(*p, win2);
        if (c.size() >= 3) {
            fillPolygon(fb, c, withAlpha(pal.neon, 150));
            drawPolygonThick(fb, c, 3.0f, pal.neon, true);
        }
    }
    drawPolygon(fb, makeRect(585, 165, 260, 260), rgb(255, 255, 255));
    drawText(fb, 506, 500, "GREY = ORIGINAL   CYAN = CLIPPED", 2, demoText());

    return saveDemo(fb, argc, argv, "04_clipping.png");
}
