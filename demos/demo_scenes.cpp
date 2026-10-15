// Demo 6: four game-style stills built only from the C++ algorithms in src/:
//   gameplay, main menu, level select, and death frame.
#include "common.h"
#include "game/sprites.h"
#include "raster/circle.h"
#include "raster/curve.h"
#include "raster/line.h"
#include "ui/ui.h"
#include <cmath>

namespace {

const int W = 960, H = 540;

void drawLevelObjects(Framebuffer& fb, const Palette& pal, int groundY, float dx, bool withPad = true) {
    drawSpike(fb, 520 + dx, (float)groundY, 48, 48, pal);
    drawSpike(fb, 572 + dx, (float)groundY, 48, 48, pal);
    drawBlock(fb, 660 + dx, groundY - 60.0f, 60, pal);
    drawBlock(fb, 720 + dx, groundY - 60.0f, 60, pal);
    drawBlock(fb, 720 + dx, groundY - 120.0f, 60, pal);
    drawSaw(fb, Vec2(832 + dx, groundY - 82.0f), 44, 0.6f, pal);
    drawPortal(fb, Vec2(915 + dx, groundY - 70.0f), 22, 56, pal.neon2);
    if (withPad) drawPad(fb, 450 + dx, (float)groundY, pal);
}

int gameplay(int argc, char** argv) {
    Framebuffer fb(W, H);
    const Palette pal = defaultPalette();
    const int groundY = 440;
    drawBackdrop(fb, 1800, groundY, pal);
    drawGround(fb, groundY, 1800, pal);
    drawLevelObjects(fb, pal, groundY, 0);
    drawOrb(fb, Vec2(395, 320), 15, 0.6f, pal);
    drawSpike(fb, 215, (float)groundY, 50, 50, pal);

    const Vec2 p0(90, 412), p1(150, 232), p2(250, 232), p3(310, 412);
    const float t = 0.55f;
    drawJumpTrail(fb, p0, p1, p2, p3, t, pal.cube);
    drawCube(fb, bezierPoint(p0, p1, p2, p3, t), 56, t * kPi / 2, pal);
    drawHud(fb, 1, 0.42f, 7, pal);
    return saveDemo(fb, argc, argv, "06_scene_gameplay.png");
}

int menu(int argc, char** argv) {
    Framebuffer fb(W, H);
    const Palette pal = defaultPalette();
    const int groundY = 462;
    drawBackdrop(fb, 900, groundY, pal);
    drawGround(fb, groundY, 900, pal);

    drawLogo(fb, W / 2, 62, 12, pal);
    drawPulseLine(fb, 200, 760, 200, 38, pal.neon);

    drawPlayButton(fb, W / 2, 295, 132, pal);
    drawTextCentered(fb, W / 2, 392, "TAP TO START", 3, rgb(240, 244, 255));

    // Side buttons: level grid (4 squares) and settings gear (a small saw).
    drawIconButton(fb, 322, 295, 78, pal.neon);
    for (int i = 0; i < 4; ++i)
        fillPolygon(fb, makeRect(322.0f - 22 + (i % 2) * 24, 295.0f - 22 + (i / 2) * 24, 20, 20), pal.neon);
    drawTextCentered(fb, 322, 350, "LEVELS", 2, rgb(200, 210, 255));
    drawIconButton(fb, 638, 295, 78, pal.neon2);
    drawSaw(fb, Vec2(638, 295), 26, 0.3f, pal);
    drawTextCentered(fb, 638, 350, "SETTINGS", 2, rgb(200, 210, 255));

    drawCube(fb, Vec2(150, groundY - 30.0f), 60, 0.0f, pal);
    drawSpike(fb, 810, (float)groundY, 52, 52, pal);
    drawSpike(fb, 862, (float)groundY, 52, 52, pal);
    drawTextCentered(fb, W / 2, 500, "CHECKPOINT 1 BUILD  -  PURE C++ RENDERER", 2, rgb(150, 160, 215));
    return saveDemo(fb, argc, argv, "07_scene_menu.png");
}

int levelSelect(int argc, char** argv) {
    Framebuffer fb(W, H);
    const Palette pal = defaultPalette();
    const int groundY = 486;
    drawBackdrop(fb, 3200, groundY, pal);
    drawGround(fb, groundY, 3200, pal);

    drawTextCentered(fb, W / 2 + 3, 33, "SELECT LEVEL", 6, rgb(6, 8, 30));
    drawTextCentered(fb, W / 2, 30, "SELECT LEVEL", 6, rgb(240, 244, 255));

    const int cw = 200, ch = 300, gap = 30, y = 100;
    const int x0 = (W - (4 * cw + 3 * gap)) / 2;
    const char* names[4] = {"PRIMER", "ESCALATION", "ADVANCED", "EXPERT"};
    const Color accents[4] = {pal.cube, pal.neon, pal.neon2, rgb(120, 122, 160)};
    const float best[4] = {1.00f, 0.64f, 0.12f, 0.0f};
    for (int i = 0; i < 4; ++i)
        drawLevelCard(fb, x0 + i * (cw + gap), y, cw, ch, names[i], i + 1, i, best[i], i == 3, accents[i], pal);

    drawTextCentered(fb, W / 2, 432, "LEVELS 1-3 COMMITTED   LEVEL 4 STRETCH GOAL", 2, rgb(200, 210, 255));
    return saveDemo(fb, argc, argv, "08_scene_level_select.png");
}

int death(int argc, char** argv) {
    Framebuffer fb(W, H);
    const Palette pal = defaultPalette();
    const int groundY = 440;
    drawBackdrop(fb, 2600, groundY, pal);
    drawGround(fb, groundY, 2600, pal);
    drawLevelObjects(fb, pal, groundY, -170, false);  // first spike now sits at x=350

    drawDeathBurst(fb, Vec2(350, 405), 0.42f, pal, ClipRect{0, 0, (double)W - 1, (double)H - 1});
    fb.fillRect(0, 0, W, H, withAlpha(pal.hazard, 26));  // red hit flash

    drawTextCentered(fb, W / 2 + 3, 143, "CRASHED AT 63%", 5, rgb(6, 8, 30));
    drawTextCentered(fb, W / 2, 140, "CRASHED AT 63%", 5, rgb(255, 235, 240));
    drawTextCentered(fb, W / 2, 205, "TAP TO RETRY", 3, pal.accent);
    drawHud(fb, 1, 0.63f, 8, pal);
    return saveDemo(fb, argc, argv, "09_scene_death.png");
}

}  // namespace

int main(int argc, char** argv) {
    int rc = 0;
    rc |= gameplay(argc, argv);
    rc |= menu(argc, argv);
    rc |= levelSelect(argc, argv);
    rc |= death(argc, argv);
    return rc;
}
