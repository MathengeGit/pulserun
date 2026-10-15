// Demo 1: line algorithms (DDA, Bresenham all octants, dashed, thick)
#include "common.h"
#include "raster/circle.h"
#include "raster/line.h"
#include "raster/polygon.h"
#include "ui/ui.h"
#include <cmath>

int main(int argc, char** argv) {
    Framebuffer fb(960, 540);
    fb.clear(demoBg());
    const Palette pal = defaultPalette();

    drawText(fb, 24, 18, "LINE ALGORITHMS", 4, demoText());

    // ---- Panel A: Bresenham starburst covering all 8 octants
    drawRoundedRect(fb, 20, 70, 440, 450, 14, demoFrame(), 2);
    drawText(fb, 36, 84, "BRESENHAM: ALL 8 OCTANTS", 2, demoDim());
    const int cx = 240, cy = 305, R = 170, rays = 48;
    for (int i = 0; i < rays; ++i) {
        const float a = 2.0f * kPi * i / rays;
        drawLineBresenham(fb, cx, cy, cx + (int)std::lround(R * std::cos(a)), cy + (int)std::lround(R * std::sin(a)),
                          lerpColor(pal.neon, pal.neon2, (float)i / rays));
    }
    fillCircle(fb, cx, cy, 6, pal.accent);

    // ---- Panel B: pixel-level view, Bresenham cells vs DDA dots
    drawRoundedRect(fb, 480, 70, 460, 200, 14, demoFrame(), 2);
    drawText(fb, 496, 84, "BRESENHAM CELLS / DDA DOTS", 2, demoDim());
    const int ox = 502, oy = 112, cell = 16, cols = 26, rows = 9;
    for (int i = 0; i <= cols; ++i) drawLineBresenham(fb, ox + i * cell, oy, ox + i * cell, oy + rows * cell, rgb(34, 40, 80));
    for (int j = 0; j <= rows; ++j) drawLineBresenham(fb, ox, oy + j * cell, ox + cols * cell, oy + j * cell, rgb(34, 40, 80));
    const auto bres = bresenhamPoints(1, 1, 24, 7);
    const auto dda = ddaPoints(1, 1, 24, 7);
    for (const IPoint& p : bres) fb.fillRect(ox + p.x * cell + 1, oy + p.y * cell + 1, cell - 1, cell - 1, withAlpha(pal.neon, 200));
    for (const IPoint& p : dda) fillCircle(fb, ox + p.x * cell + cell / 2, oy + p.y * cell + cell / 2, 3, pal.accent);
    drawLineBresenham(fb, ox + 1 * cell + cell / 2, oy + 1 * cell + cell / 2, ox + 24 * cell + cell / 2, oy + 7 * cell + cell / 2,
                      rgb(255, 255, 255));

    // ---- Panel C: dashed lines
    drawRoundedRect(fb, 480, 285, 460, 110, 14, demoFrame(), 2);
    drawText(fb, 496, 299, "DASHED LINES", 2, demoDim());
    drawDashedLine(fb, 500, 335, 920, 335, pal.neon, 14, 7);
    drawDashedLine(fb, 500, 358, 920, 385, pal.neon2, 5, 5);
    drawDashedLine(fb, 500, 385, 700, 345, pal.accent, 2, 4);

    // ---- Panel D: thick lines (filled quads)
    drawRoundedRect(fb, 480, 410, 460, 110, 14, demoFrame(), 2);
    drawText(fb, 496, 424, "THICK LINES: WIDTH 2 / 6 / 12 / 20", 2, demoDim());
    const float widths[4] = {2, 6, 12, 20};
    for (int i = 0; i < 4; ++i) {
        const float x = 540.0f + i * 110.0f;
        drawThickLine(fb, Vec2(x, 505), Vec2(x + 50, 452), widths[i], lerpColor(pal.neon, pal.neon2, i / 3.0f), true);
    }

    return saveDemo(fb, argc, argv, "01_lines.png");
}
