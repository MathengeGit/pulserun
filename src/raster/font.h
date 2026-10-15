#pragma once
#include "core/framebuffer.h"
#include <string>

// 5x7 bitmap font. Each lit font pixel is drawn as a scale x scale block, so
// text is rendered with nothing but rectangles (and therefore our own raster code).
int textWidth(const std::string& text, int scale);
void drawText(Framebuffer& fb, int x, int y, const std::string& text, int scale, Color c);
void drawTextShadow(Framebuffer& fb, int x, int y, const std::string& text, int scale, Color c, Color shadow,
                    int offset);
void drawTextCentered(Framebuffer& fb, int cx, int y, const std::string& text, int scale, Color c);
