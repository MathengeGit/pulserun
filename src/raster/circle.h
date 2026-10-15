#pragma once
#include "core/framebuffer.h"
#include <vector>

// Half-width of a midpoint circle at each row offset 0..r (used for filling).
std::vector<int> circleHalfWidths(int r);

void drawCircle(Framebuffer& fb, int cx, int cy, int r, Color c);
void drawCircleThick(Framebuffer& fb, int cx, int cy, int r, int thickness, Color c);
void fillCircle(Framebuffer& fb, int cx, int cy, int r, Color c);

// Angles are in degrees, counter-clockwise as seen on screen (0 = right, 90 = up).
void drawArc(Framebuffer& fb, int cx, int cy, int r, float startDeg, float endDeg, Color c);
void drawArcThick(Framebuffer& fb, int cx, int cy, int r, float startDeg, float endDeg, int thickness, Color c);

void drawEllipse(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c);
void fillEllipse(Framebuffer& fb, int cx, int cy, int rx, int ry, Color c);
