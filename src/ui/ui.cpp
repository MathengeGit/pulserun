#include "ui/ui.h"
#include "raster/circle.h"
#include "raster/curve.h"
#include "raster/font.h"
#include "raster/line.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
int ir(float v) { return (int)std::lround(v); }

// 1px rounded-rectangle outline: four lines + four quarter arcs.
void roundedOutline1px(Framebuffer& fb, int x, int y, int w, int h, int r, Color c) {
    r = std::min(r, std::min(w, h) / 2);
    if (r < 1) {
        drawPolygon(fb, makeRect((float)x, (float)y, (float)w - 1, (float)h - 1), c);
        return;
    }
    const int xr = x + w - 1, yb = y + h - 1;
    if (x + r <= xr - r) {
        drawLineBresenham(fb, x + r, y, xr - r, y, c);    // top
        drawLineBresenham(fb, x + r, yb, xr - r, yb, c);  // bottom
    }
    if (y + r <= yb - r) {
        drawLineBresenham(fb, x, y + r, x, yb - r, c);    // left
        drawLineBresenham(fb, xr, y + r, xr, yb - r, c);  // right
    }
    drawArc(fb, x + r, y + r, r, 90, 180, c);         // top-left
    drawArc(fb, xr - r, y + r, r, 0, 90, c);          // top-right
    drawArc(fb, x + r, yb - r, r, 180, 270, c);       // bottom-left
    drawArc(fb, xr - r, yb - r, r, 270, 360, c);      // bottom-right
}
}  // namespace

void fillRoundedRect(Framebuffer& fb, int x, int y, int w, int h, int r, Color c) {
    r = std::min(r, std::min(w, h) / 2);
    if (r < 1) {
        fb.fillRect(x, y, w, h, c);
        return;
    }
    const std::vector<int> hw = circleHalfWidths(r);  // corner profile from the midpoint circle
    const int cxL = x + r, cxR = x + w - 1 - r;
    for (int i = 0; i < h; ++i) {
        int dy = 0;
        if (i < r) dy = r - i;
        else if (i > h - 1 - r) dy = i - (h - 1 - r);
        fb.hLine(cxL - hw[dy], cxR + hw[dy], y + i, c);  // one span per row, no overlap
    }
}

void drawRoundedRect(Framebuffer& fb, int x, int y, int w, int h, int r, Color c, int thickness) {
    for (int k = 0; k < thickness; ++k) roundedOutline1px(fb, x + k, y + k, w - 2 * k, h - 2 * k, std::max(1, r - k), c);
}

void drawProgressBar(Framebuffer& fb, int x, int y, int w, int h, float progress, Color fill, Color track, Color edge) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    const int r = h / 2;
    fillRoundedRect(fb, x, y, w, h, r, track);
    const int fw = ir((w - 4) * progress);
    if (fw > 2) {
        // Glow behind the fill, then the fill itself, then a highlight strip.
        fillRoundedRect(fb, x + 2, y + 2, fw, h - 4, std::max(1, r - 2), fill);
        fb.fillRect(x + 2 + r / 2, y + 4, std::max(0, fw - r), 2, withAlpha(rgb(255, 255, 255), 90));
    }
    drawRoundedRect(fb, x, y, w, h, r, edge, 2);
}

void drawPlayButton(Framebuffer& fb, int cx, int cy, int size, const Palette& pal) {
    const int x = cx - size / 2, y = cy - size / 2;
    for (int i = 5; i >= 1; --i)
        fillRoundedRect(fb, x - i * 4, y - i * 4, size + i * 8, size + i * 8, size / 5 + i * 4, withAlpha(pal.cube, 14));
    fillRoundedRect(fb, x, y, size, size, size / 5, pal.cube);
    fillRoundedRect(fb, x + 8, y + 8, size - 16, size - 16, size / 6, shade(pal.cube, 0.78f));
    drawRoundedRect(fb, x, y, size, size, size / 5, rgb(8, 24, 14), 4);
    // Play triangle.
    const float s = size * 0.30f;
    const Polygon tri{Vec2(cx - s * 0.7f, cy - s), Vec2(cx + s * 1.1f, (float)cy), Vec2(cx - s * 0.7f, cy + s)};
    fillPolygon(fb, tri, rgb(235, 255, 238));
    drawPolygonThick(fb, tri, 3.0f, rgb(8, 24, 14), true);
}

void drawIconButton(Framebuffer& fb, int cx, int cy, int size, Color accent) {
    const int x = cx - size / 2, y = cy - size / 2;
    for (int i = 3; i >= 1; --i)
        fillRoundedRect(fb, x - i * 3, y - i * 3, size + i * 6, size + i * 6, size / 4 + i * 3, withAlpha(accent, 20));
    fillRoundedRect(fb, x, y, size, size, size / 4, rgb(14, 16, 48));
    drawRoundedRect(fb, x, y, size, size, size / 4, accent, 3);
}

void drawFaceIcon(Framebuffer& fb, int cx, int cy, int r, int difficulty, Color c) {
    fillCircle(fb, cx, cy, r + r / 5, withAlpha(c, 40));
    fillCircle(fb, cx, cy, r, c);
    drawCircleThick(fb, cx, cy, r, 3, rgb(12, 14, 40));
    const Color ink = rgb(12, 14, 40);
    fillEllipse(fb, cx - r * 35 / 100, cy - r / 5, std::max(2, r / 9), std::max(3, r / 5), ink);
    fillEllipse(fb, cx + r * 35 / 100, cy - r / 5, std::max(2, r / 9), std::max(3, r / 5), ink);
    if (difficulty == 0) {  // smile
        drawArcThick(fb, cx, cy, r * 45 / 100, 205, 335, 4, ink);
    } else if (difficulty == 1) {  // neutral
        drawThickLine(fb, Vec2((float)cx - r * 0.3f, cy + r * 0.38f), Vec2((float)cx + r * 0.3f, cy + r * 0.38f), 4.0f, ink, true);
    } else {  // frown
        drawArcThick(fb, cx, cy + r * 75 / 100, r * 40 / 100, 40, 140, 4, ink);
    }
}

void drawPadlock(Framebuffer& fb, int cx, int cy, int size, Color c) {
    const int w = size, h = size * 3 / 4;
    const int bx = cx - w / 2, by = cy - h / 4;
    drawArcThick(fb, cx, by, w / 3, 0, 180, 5, c);  // shackle
    drawThickLine(fb, Vec2((float)cx - w / 3, (float)by), Vec2((float)cx - w / 3, (float)by + 6), 5.0f, c);
    drawThickLine(fb, Vec2((float)cx + w / 3, (float)by), Vec2((float)cx + w / 3, (float)by + 6), 5.0f, c);
    fillRoundedRect(fb, bx, by, w, h, 5, c);
    fillCircle(fb, cx, by + h / 2 - 2, std::max(2, size / 9), rgb(12, 14, 40));
    fb.fillRect(cx - 1, by + h / 2, 3, size / 6, rgb(12, 14, 40));
}

void drawLevelCard(Framebuffer& fb, int x, int y, int w, int h, const std::string& name, int levelNo, int difficulty,
                   float best, bool locked, Color accent, const Palette& pal) {
    const Color col = locked ? rgb(110, 112, 150) : accent;
    for (int i = 3; i >= 1; --i) fillRoundedRect(fb, x - i * 3, y - i * 3, w + i * 6, h + i * 6, 16 + i * 3, withAlpha(col, 16));
    fillRoundedRect(fb, x, y, w, h, 16, rgb(14, 16, 48));
    fillRoundedRect(fb, x + 6, y + 6, w - 12, 34, 10, withAlpha(col, 60));
    drawRoundedRect(fb, x, y, w, h, 16, col, 3);

    char buf[32];
    std::snprintf(buf, sizeof buf, "LEVEL %d", levelNo);
    drawTextCentered(fb, x + w / 2, y + 15, buf, 2, rgb(240, 240, 255));

    if (locked) {
        drawPadlock(fb, x + w / 2, y + h / 2 - 20, 54, col);
    } else {
        drawFaceIcon(fb, x + w / 2, y + h / 2 - 22, 46, difficulty, col);
    }
    drawTextCentered(fb, x + w / 2, y + h - 98, name, 3, locked ? col : rgb(255, 255, 255));

    if (locked) {
        drawTextCentered(fb, x + w / 2, y + h - 56, "LOCKED", 2, col);
        drawProgressBar(fb, x + 18, y + h - 34, w - 36, 16, 0.0f, col, rgb(8, 10, 30), col);
    } else {
        std::snprintf(buf, sizeof buf, "BEST %d%%", (int)std::lround(best * 100));
        drawTextCentered(fb, x + w / 2, y + h - 56, buf, 2, rgb(220, 224, 255));
        drawProgressBar(fb, x + 18, y + h - 34, w - 36, 16, best, col, rgb(8, 10, 30), col);
    }
    (void)pal;
}

void drawPulseLine(Framebuffer& fb, int x0, int x1, int y, float amp, Color c) {
    // A heartbeat (ECG) polyline: flat, bump, big spike, undershoot, flat.
    const float w = (float)(x1 - x0);
    const float fy = (float)y;
    std::vector<Vec2> pts = {
        Vec2(x0 + w * 0.00f, fy),         Vec2(x0 + w * 0.30f, fy),
        Vec2(x0 + w * 0.34f, fy - amp * 0.18f), Vec2(x0 + w * 0.38f, fy),
        Vec2(x0 + w * 0.42f, fy),         Vec2(x0 + w * 0.455f, fy + amp * 0.25f),
        Vec2(x0 + w * 0.50f, fy - amp),   Vec2(x0 + w * 0.55f, fy + amp * 0.55f),
        Vec2(x0 + w * 0.585f, fy),        Vec2(x0 + w * 0.63f, fy),
        Vec2(x0 + w * 0.68f, fy - amp * 0.28f), Vec2(x0 + w * 0.74f, fy),
        Vec2(x0 + w * 1.00f, fy)};
    drawGlowPolyline(fb, pts, c, 4.0f, 4);
}

void drawLogo(Framebuffer& fb, int cx, int y, int scale, const Palette& pal) {
    const std::string a = "PULSE", b = "RUN";
    const int total = textWidth(a + b, scale);
    const int x = cx - total / 2;
    // Soft glow: the text drawn several times, offset, with low alpha.
    for (int g = 3; g >= 1; --g) {
        drawText(fb, x - g * 2, y, a, scale, withAlpha(pal.neon, 14));
        drawText(fb, x + g * 2, y, a, scale, withAlpha(pal.neon, 14));
    }
    drawTextShadow(fb, x, y, a, scale, pal.neon, rgb(6, 8, 30), std::max(3, scale / 2));
    drawTextShadow(fb, x + textWidth(a, scale) + scale, y, b, scale, pal.neon2, rgb(6, 8, 30), std::max(3, scale / 2));
}

void drawHud(Framebuffer& fb, int levelNo, float progress, int attempt, const Palette& pal) {
    char buf[40];
    std::snprintf(buf, sizeof buf, "LEVEL %02d", levelNo);
    drawTextShadow(fb, 26, 20, buf, 2, rgb(235, 240, 255), rgb(6, 8, 30), 2);
    std::snprintf(buf, sizeof buf, "ATTEMPT %d", attempt);
    drawTextShadow(fb, fb.width() - 26 - textWidth(buf, 2), 20, buf, 2, rgb(235, 240, 255), rgb(6, 8, 30), 2);

    const int bw = 440, bh = 18, bx = (fb.width() - bw) / 2 - 24, by = 18;
    drawProgressBar(fb, bx, by, bw, bh, progress, pal.neon, rgb(8, 10, 34), rgb(235, 240, 255));
    std::snprintf(buf, sizeof buf, "%d%%", (int)std::lround(progress * 100));
    drawTextShadow(fb, bx + bw + 14, 20, buf, 2, pal.neon, rgb(6, 8, 30), 2);
}
