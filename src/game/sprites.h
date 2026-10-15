#pragma once
#include "core/framebuffer.h"
#include "raster/clip.h"
#include "raster/polygon.h"
#include <vector>

// Colour scheme shared by the game scenes and the UI.
struct Palette {
    Color bgTop, bgBottom;
    Color neon;      // primary accent (cyan)
    Color neon2;     // secondary accent (magenta)
    Color accent;    // highlight (yellow)
    Color hazard;    // spikes / saws (hot pink-red)
    Color cube;      // player
    Color dark;      // obstacle fill
    Color skyline, hillFar, hillNear;
    Color groundTop, groundBottom;
};
Palette defaultPalette();

// ---- Background / environment (parallax layers) ----
void drawBackdrop(Framebuffer& fb, float scroll, int groundY, const Palette& pal);
void drawGround(Framebuffer& fb, int groundY, float scroll, const Palette& pal);

// ---- Obstacles ----
void drawGlowPolygon(Framebuffer& fb, const Polygon& poly, Color neon, Color fill, float thickness = 3.0f,
                     int glowLayers = 4);
void drawGlowPolyline(Framebuffer& fb, const std::vector<Vec2>& pts, Color color, float width, int glowLayers = 4);
void drawBlock(Framebuffer& fb, float x, float y, float size, const Palette& pal);
void drawSpike(Framebuffer& fb, float centerX, float baseY, float w, float h, const Palette& pal);
void drawSaw(Framebuffer& fb, Vec2 center, float radius, float angle, const Palette& pal);
void drawOrb(Framebuffer& fb, Vec2 center, int radius, float pulse, const Palette& pal);
void drawPad(Framebuffer& fb, float centerX, float groundY, const Palette& pal);
void drawPortal(Framebuffer& fb, Vec2 center, float halfW, float halfH, Color color);

// ---- Player ----
void drawCube(Framebuffer& fb, Vec2 center, float size, float angle, const Palette& pal);
// Fading trail along a cubic Bezier jump arc from t=0 to t=tEnd.
void drawJumpTrail(Framebuffer& fb, Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float tEnd, Color c);
// Death animation: polygon shards flying outward plus expanding shock-wave rings.
// t in [0,1]. Shards are clipped to the viewport with Sutherland-Hodgman.
void drawDeathBurst(Framebuffer& fb, Vec2 center, float t, const Palette& pal, const ClipRect& viewport);
