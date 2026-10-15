#pragma once
#include "core/framebuffer.h"
#include "math/vec2.h"
#include <vector>

struct IPoint {
    int x, y;
};

// Pixel generators (return the pixels a line touches, in order from start to end).
std::vector<IPoint> ddaPoints(int x0, int y0, int x1, int y1);
std::vector<IPoint> bresenhamPoints(int x0, int y0, int x1, int y1);

// Drawing helpers built on the generators.
void drawLineDDA(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c);
void drawLineBresenham(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c);

// Dashed line: `dash` pixels on, `gap` pixels off, walking the Bresenham pixels.
void drawDashedLine(Framebuffer& fb, int x0, int y0, int x1, int y1, Color c, int dash, int gap);

// Thick line: builds a rectangle around the segment and fills it with the
// scanline polygon filler, so every pixel is blended exactly once.
void drawThickLine(Framebuffer& fb, Vec2 a, Vec2 b, float width, Color c, bool roundCaps = false);
