#include "game/sprites.h"
#include "raster/circle.h"
#include "raster/curve.h"
#include "raster/line.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

// Small deterministic hash so scenery looks random but is identical every run.
float hash01(int a, int b) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFF) / 16777215.0f;
}

int ir(float v) { return (int)std::lround(v); }

// One parallax hill layer. Control points sit on a fixed screen grid; their
// heights come from a periodic function indexed by world position, and a
// Catmull-Rom spline joins them smoothly.
void drawHillLayer(Framebuffer& fb, int baseY, float amp, float scrollShift, int layerSeed, int groundY, Color c) {
    const float spacing = 130.0f;
    const int k = (int)std::floor(scrollShift / spacing);
    const float frac = scrollShift - k * spacing;
    std::vector<Vec2> ctrl;
    for (int i = 0; i * spacing - spacing - frac < fb.width() + 2 * spacing; ++i) {
        const float x = i * spacing - spacing - frac;
        const int j = i + k;
        const float h = 0.5f + 0.35f * std::sin(j * 0.9f + layerSeed) + 0.15f * std::sin(j * 2.3f + layerSeed * 1.7f);
        ctrl.push_back(Vec2(x, baseY - amp * h));
    }
    Polygon poly = sampleCatmullRom(ctrl, 14);
    poly.push_back(Vec2(poly.back().x, (float)groundY));
    poly.push_back(Vec2(poly.front().x, (float)groundY));
    fillPolygon(fb, poly, c);
}

void drawSkyline(Framebuffer& fb, int groundY, float scrollShift, const Palette& pal) {
    const float tile = 1200.0f;
    const float off = std::fmod(scrollShift, tile);
    for (int rep = -1; rep <= 1; ++rep) {
        for (int i = 0; i < 20; ++i) {
            const float bw = 36.0f + hash01(i, 1) * 34.0f;
            const float bh = 70.0f + hash01(i, 2) * 150.0f;
            const float bx = i * 60.0f + rep * tile - off;
            if (bx > fb.width() || bx + bw < 0) continue;
            fillPolygon(fb, makeRect(bx, groundY - bh, bw, bh), pal.skyline);
            // Lit windows
            for (int wy = 0; wy < (int)(bh / 20) - 1; ++wy)
                for (int wx = 0; wx < (int)(bw / 14); ++wx)
                    if (hash01(i * 31 + wx, wy + 7) > 0.62f)
                        fb.fillRect(ir(bx + 6 + wx * 14), ir(groundY - bh + 10 + wy * 20), 6, 8, withAlpha(pal.neon2, 120));
        }
    }
}

}  // namespace

Palette defaultPalette() {
    Palette p;
    p.bgTop = rgb(16, 22, 74);
    p.bgBottom = rgb(86, 36, 130);
    p.neon = rgb(0, 238, 255);
    p.neon2 = rgb(255, 74, 200);
    p.accent = rgb(255, 224, 70);
    p.hazard = rgb(255, 70, 110);
    p.cube = rgb(86, 255, 98);
    p.dark = rgb(12, 14, 40);
    p.skyline = rgb(30, 22, 78);
    p.hillFar = rgb(46, 30, 104);
    p.hillNear = rgb(26, 20, 72);
    p.groundTop = rgb(18, 26, 86);
    p.groundBottom = rgb(8, 10, 34);
    return p;
}

// ------------------------------------------------------------------ environment
void drawBackdrop(Framebuffer& fb, float scroll, int groundY, const Palette& pal) {
    for (int y = 0; y < fb.height(); ++y)
        fb.hLine(0, fb.width() - 1, y, lerpColor(pal.bgTop, pal.bgBottom, (float)y / (fb.height() - 1)));

    // Far layer: big soft discs (slowest parallax).
    for (int i = 0; i < 7; ++i) {
        float x = std::fmod(i * 190.0f - scroll * 0.08f, 1330.0f);
        if (x < 0) x += 1330.0f;
        x -= 200.0f;
        const float y = 90.0f + hash01(i, 5) * 230.0f;
        const int r = 40 + (int)(hash01(i, 6) * 60);
        fillCircle(fb, ir(x), ir(y), r, withAlpha(pal.neon2, 16));
        fillCircle(fb, ir(x), ir(y), r * 2 / 3, withAlpha(pal.neon2, 14));
    }
    drawSkyline(fb, groundY, scroll * 0.30f, pal);
    drawHillLayer(fb, groundY - 40, 70.0f, scroll * 0.45f, 3, groundY, pal.hillFar);
    drawHillLayer(fb, groundY - 5, 55.0f, scroll * 0.70f, 8, groundY, pal.hillNear);
}

void drawGround(Framebuffer& fb, int groundY, float scroll, const Palette& pal) {
    for (int y = groundY; y < fb.height(); ++y)
        fb.hLine(0, fb.width() - 1, y,
                 lerpColor(pal.groundTop, pal.groundBottom, (float)(y - groundY) / (fb.height() - groundY)));
    // Glow above the line, then the bright line itself.
    for (int i = 1; i <= 7; ++i) fb.hLine(0, fb.width() - 1, groundY - i, withAlpha(pal.neon, 40 - i * 5));
    fb.fillRect(0, groundY, fb.width(), 3, pal.neon);

    // Vertical tick marks scroll with the world. Clipped to the ground band (Cohen-Sutherland).
    const ClipRect band{0, (double)groundY + 6, (double)fb.width() - 1, (double)fb.height() - 1};
    const float off = std::fmod(scroll, 60.0f);
    for (float x = -off; x < fb.width() + 60; x += 60.0f)
        drawLineClipped(fb, x, groundY - 20, x - 40, fb.height() + 20, band, withAlpha(pal.neon, 55));
}

// ------------------------------------------------------------------ obstacles
void drawGlowPolygon(Framebuffer& fb, const Polygon& poly, Color neon, Color fill, float thickness, int glowLayers) {
    for (int i = glowLayers; i >= 1; --i) drawPolygonThick(fb, poly, thickness + i * 3.0f, withAlpha(neon, 22));
    fillPolygon(fb, poly, fill);
    drawPolygonThick(fb, poly, thickness, neon, true);
}

void drawGlowPolyline(Framebuffer& fb, const std::vector<Vec2>& pts, Color color, float width, int glowLayers) {
    for (int i = glowLayers; i >= 1; --i) drawPolylineThick(fb, pts, width + i * 4.0f, withAlpha(color, 22), true);
    drawPolylineThick(fb, pts, width, color, true);
}

void drawBlock(Framebuffer& fb, float x, float y, float size, const Palette& pal) {
    const Polygon outer = makeRect(x, y, size, size);
    drawGlowPolygon(fb, outer, pal.neon, pal.dark, 3.0f, 2);
    // Inner inset square + corner studs give the block its tiled look.
    drawPolygonThick(fb, makeRect(x + 11, y + 11, size - 22, size - 22), 2.0f, withAlpha(pal.neon, 110));
    const float s = 6.0f;
    for (const Vec2& c : {Vec2(x + 4, y + 4), Vec2(x + size - 4 - s, y + 4), Vec2(x + 4, y + size - 4 - s),
                          Vec2(x + size - 4 - s, y + size - 4 - s)})
        fillPolygon(fb, makeRect(c.x, c.y, s, s), withAlpha(pal.neon, 170));
}

void drawSpike(Framebuffer& fb, float centerX, float baseY, float w, float h, const Palette& pal) {
    const Polygon tri{Vec2(centerX - w / 2, baseY), Vec2(centerX, baseY - h), Vec2(centerX + w / 2, baseY)};
    drawGlowPolygon(fb, tri, pal.hazard, pal.dark, 3.0f, 4);
    // Inner highlight triangle.
    const Polygon inner{Vec2(centerX - w * 0.2f, baseY - 6), Vec2(centerX, baseY - h * 0.55f),
                        Vec2(centerX + w * 0.2f, baseY - 6)};
    fillPolygon(fb, inner, withAlpha(pal.hazard, 90));
}

void drawSaw(Framebuffer& fb, Vec2 center, float radius, float angle, const Palette& pal) {
    const Polygon teeth = starPolygon(center, radius, radius * 0.74f, 9, angle);
    drawGlowPolygon(fb, teeth, pal.hazard, pal.dark, 3.0f, 4);
    const int cx = ir(center.x), cy = ir(center.y), r = (int)radius;
    drawCircleThick(fb, cx, cy, r / 2, 3, withAlpha(pal.hazard, 200));
    fillCircle(fb, cx, cy, r / 5, pal.hazard);
    // Spokes rotate with the saw (rotation transform).
    for (int i = 0; i < 3; ++i) {
        const Vec2 d = rotate(Vec2(0, -(float)r * 0.5f), angle + i * 2.0f * kPi / 3.0f);
        drawThickLine(fb, center, center + d, 3.0f, withAlpha(pal.hazard, 200));
    }
}

void drawOrb(Framebuffer& fb, Vec2 center, int radius, float pulse, const Palette& pal) {
    const int cx = ir(center.x), cy = ir(center.y);
    fillCircle(fb, cx, cy, (int)(radius * (1.9f + 0.3f * pulse)), withAlpha(pal.accent, 22));
    fillCircle(fb, cx, cy, (int)(radius * (1.5f + 0.2f * pulse)), withAlpha(pal.accent, 38));
    drawCircleThick(fb, cx, cy, (int)(radius * (1.25f + 0.2f * pulse)), 3, withAlpha(pal.accent, 220));
    fillCircle(fb, cx, cy, radius, withAlpha(pal.accent, 255));
    fillCircle(fb, cx, cy, radius * 2 / 3, rgb(255, 250, 200));
}

void drawPad(Framebuffer& fb, float centerX, float groundY, const Palette& pal) {
    const int cx = ir(centerX), gy = ir(groundY);
    fillEllipse(fb, cx, gy, 44, 16, withAlpha(pal.accent, 30));
    fillEllipse(fb, cx, gy, 32, 10, withAlpha(pal.accent, 90));
    fillEllipse(fb, cx, gy, 24, 6, pal.accent);
    drawEllipse(fb, cx, gy, 24, 6, rgb(255, 250, 200));
}

void drawPortal(Framebuffer& fb, Vec2 c, float hw, float hh, Color color) {
    // Each side of the portal oval is one cubic Bezier.
    for (int ring = 0; ring < 3; ++ring) {
        const float k = 1.0f - ring * 0.28f;
        const float w = hw * k, h = hh * k;
        const Color col = withAlpha(color, ring == 0 ? 255 : 170 - ring * 50);
        const auto right = sampleBezier(Vec2(c.x, c.y - h), Vec2(c.x + w * 1.45f, c.y - h * 0.95f),
                                        Vec2(c.x + w * 1.45f, c.y + h * 0.95f), Vec2(c.x, c.y + h), 40);
        const auto left = sampleBezier(Vec2(c.x, c.y - h), Vec2(c.x - w * 1.45f, c.y - h * 0.95f),
                                       Vec2(c.x - w * 1.45f, c.y + h * 0.95f), Vec2(c.x, c.y + h), 40);
        if (ring == 0) {
            drawPolylineThick(fb, right, 11.0f, withAlpha(color, 30), false);
            drawPolylineThick(fb, left, 11.0f, withAlpha(color, 30), false);
        }
        drawPolylineThick(fb, right, ring == 0 ? 5.0f : 3.0f, col);
        drawPolylineThick(fb, left, ring == 0 ? 5.0f : 3.0f, col);
    }
}

// ------------------------------------------------------------------ player
void drawCube(Framebuffer& fb, Vec2 c, float s, float angle, const Palette& pal) {
    auto toWorld = [&](float lx, float ly) { return c + rotate(Vec2(lx, ly), angle); };
    auto rectPoly = [&](float cx, float cy, float w, float h) {
        return Polygon{toWorld(cx - w / 2, cy - h / 2), toWorld(cx + w / 2, cy - h / 2),
                       toWorld(cx + w / 2, cy + h / 2), toWorld(cx - w / 2, cy + h / 2)};
    };
    for (int i = 4; i >= 1; --i) fillPolygon(fb, rectPoly(0, 0, s + i * 9.0f, s + i * 9.0f), withAlpha(pal.cube, 18));
    const Polygon body = rectPoly(0, 0, s, s);
    fillPolygon(fb, body, pal.cube);
    fillPolygon(fb, rectPoly(0, 0, s * 0.64f, s * 0.64f), shade(pal.cube, 0.78f));
    drawPolygonThick(fb, body, 4.0f, rgb(8, 24, 14), true);
    // Face: two eyes and a mouth, all rotated with the cube.
    const Color ink = rgb(8, 24, 14);
    fillPolygon(fb, rectPoly(-0.20f * s, -0.10f * s, 0.17f * s, 0.20f * s), ink);
    fillPolygon(fb, rectPoly(0.20f * s, -0.10f * s, 0.17f * s, 0.20f * s), ink);
    fillPolygon(fb, rectPoly(0, 0.22f * s, 0.46f * s, 0.07f * s), ink);
}

void drawJumpTrail(Framebuffer& fb, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float tEnd, Color c) {
    const int segs = 36;
    const auto pts = sampleBezier(p0, p1, p2, p3, segs, tEnd);
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        const float f = (float)i / (pts.size() - 1);  // 0 = oldest, 1 = newest
        drawThickLine(fb, pts[i], pts[i + 1], 3.0f + 14.0f * f, withAlpha(c, (int)(10 + 90 * f)));
    }
}

void drawDeathBurst(Framebuffer& fb, Vec2 center, float t, const Palette& pal, const ClipRect& viewport) {
    const int alpha = (int)(255 * (1.0f - t));
    // Shock-wave rings (midpoint circles).
    for (int ring = 0; ring < 3; ++ring) {
        const int r = (int)(t * (200 + ring * 70)) + ring * 10;
        drawCircleThick(fb, ir(center.x), ir(center.y), r, 4 - ring, withAlpha(pal.cube, std::max(0, alpha - ring * 60)));
    }
    // Shards.
    const int n = 16;
    for (int i = 0; i < n; ++i) {
        const float a = 2.0f * kPi * i / n + hash01(i, 11) * 0.5f;
        const float dist = t * (110.0f + hash01(i, 12) * 190.0f);
        const Vec2 pos = center + Vec2(std::cos(a), std::sin(a)) * dist + Vec2(0, 140.0f * t * t);  // gravity pull
        const float size = 9.0f + hash01(i, 13) * 15.0f;
        const float spin = a + t * (3.0f + hash01(i, 14) * 5.0f);
        Polygon shard{Vec2(0, -size), Vec2(size * 0.9f, size * 0.7f), Vec2(-size * 0.8f, size * 0.6f)};
        for (Vec2& v : shard) v = pos + rotate(v, spin);
        const Color col = (i % 3 == 0) ? pal.neon : pal.cube;
        fillPolygonClipped(fb, shard, viewport, withAlpha(col, alpha));
    }
}
