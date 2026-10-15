#pragma once
#include "game/sprites.h"
#include <string>

// ---- Shapes built from lines + arcs + circles ----
void fillRoundedRect(Framebuffer& fb, int x, int y, int w, int h, int r, Color c);
void drawRoundedRect(Framebuffer& fb, int x, int y, int w, int h, int r, Color c, int thickness = 2);

// ---- Widgets ----
void drawProgressBar(Framebuffer& fb, int x, int y, int w, int h, float progress, Color fill, Color track, Color edge);
void drawPlayButton(Framebuffer& fb, int cx, int cy, int size, const Palette& pal);
void drawIconButton(Framebuffer& fb, int cx, int cy, int size, Color accent);
// difficulty: 0 = easy (smile), 1 = normal (flat), 2 = hard (frown)
void drawFaceIcon(Framebuffer& fb, int cx, int cy, int r, int difficulty, Color c);
void drawPadlock(Framebuffer& fb, int cx, int cy, int size, Color c);
void drawLevelCard(Framebuffer& fb, int x, int y, int w, int h, const std::string& name, int levelNo, int difficulty,
                   float best, bool locked, Color accent, const Palette& pal);

// ---- Screens ----
void drawLogo(Framebuffer& fb, int cx, int y, int scale, const Palette& pal);
void drawPulseLine(Framebuffer& fb, int x0, int x1, int y, float amplitude, Color c);
void drawHud(Framebuffer& fb, int levelNo, float progress, int attempt, const Palette& pal);
