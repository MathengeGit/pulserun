// Demo 2: circles, ellipses and arcs (midpoint algorithms)
#include "common.h"
#include "raster/circle.h"
#include "raster/line.h"
#include "ui/ui.h"

int main(int argc, char** argv) {
    Framebuffer fb(960, 540);
    fb.clear(demoBg());
    const Palette pal = defaultPalette();

    drawText(fb, 24, 18, "CIRCLES, ELLIPSES AND ARCS", 4, demoText());

    // ---- Panel A: midpoint circle
    drawRoundedRect(fb, 20, 70, 300, 240, 14, demoFrame(), 2);
    drawText(fb, 36, 84, "MIDPOINT CIRCLE", 2, demoDim());
    drawCircle(fb, 100, 195, 22, pal.accent);
    drawCircle(fb, 100, 195, 44, pal.neon2);
    drawCircle(fb, 100, 195, 66, pal.neon);
    fillCircle(fb, 225, 195, 56, withAlpha(pal.neon, 120));
    drawCircleThick(fb, 225, 195, 56, 5, pal.neon);

    // ---- Panel B: midpoint ellipse
    drawRoundedRect(fb, 340, 70, 300, 240, 14, demoFrame(), 2);
    drawText(fb, 356, 84, "MIDPOINT ELLIPSE", 2, demoDim());
    drawEllipse(fb, 430, 160, 72, 32, pal.neon);
    drawEllipse(fb, 430, 160, 50, 20, pal.neon2);
    fillEllipse(fb, 560, 165, 62, 42, withAlpha(pal.neon2, 130));
    drawEllipse(fb, 560, 165, 62, 42, pal.neon2);
    fillEllipse(fb, 490, 262, 120, 14, withAlpha(pal.accent, 110));
    drawEllipse(fb, 490, 262, 120, 14, pal.accent);

    // ---- Panel C: arcs
    drawRoundedRect(fb, 660, 70, 280, 240, 14, demoFrame(), 2);
    drawText(fb, 676, 84, "ARCS", 2, demoDim());
    drawArcThick(fb, 735, 185, 62, 5, 80, 7, pal.neon);
    drawArcThick(fb, 735, 185, 62, 100, 170, 7, pal.neon2);
    drawArcThick(fb, 735, 185, 62, 190, 260, 7, pal.accent);
    drawArcThick(fb, 735, 185, 62, 280, 350, 7, pal.cube);
    // Gauge: background arc + 60% fill (240 degree sweep)
    drawArcThick(fb, 865, 205, 52, 330, 210, 8, rgb(40, 46, 90));
    drawArcThick(fb, 865, 205, 52, 66, 210, 8, pal.neon);

    // ---- Panel D: game objects built from circles and ellipses
    drawRoundedRect(fb, 20, 325, 920, 195, 14, demoFrame(), 2);
    drawText(fb, 36, 339, "PULSERUN OBJECTS: ORB / PAD / DIFFICULTY FACES / DEATH RINGS", 2, demoDim());
    drawOrb(fb, Vec2(110, 430), 22, 0.6f, pal);
    fb.fillRect(180, 480, 160, 3, pal.neon);
    drawPad(fb, 260, 480, pal);
    drawFaceIcon(fb, 430, 430, 40, 0, pal.cube);
    drawFaceIcon(fb, 535, 430, 40, 1, pal.accent);
    drawFaceIcon(fb, 640, 430, 40, 2, pal.hazard);
    for (int ring = 0; ring < 3; ++ring)
        drawCircleThick(fb, 820, 430, 20 + ring * 24, 5 - ring, withAlpha(pal.cube, 255 - ring * 80));
    fillCircle(fb, 820, 430, 8, pal.cube);

    return saveDemo(fb, argc, argv, "02_circles.png");
}
