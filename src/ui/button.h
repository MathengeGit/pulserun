#pragma once
#include "game/sprites.h"
#include <string>
#include <vector>

// A text button drawn with the existing rounded-rect + bitmap-font widgets.
struct Button {
    int x = 0, y = 0, w = 0, h = 0;
    std::string label;
    Color accent{0, 238, 255, 255};
    bool enabled = true;

    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// `focused` draws the keyboard-focus ring (a white outline offset outside the button).
void drawButton(Framebuffer& fb, const Button& b, bool focused);

// Index of the enabled button under (px, py), or -1.
int buttonAt(const std::vector<Button>& buttons, int px, int py);

// Move focus by `delta` (wrapping), skipping disabled buttons.
int moveFocus(const std::vector<Button>& buttons, int focus, int delta);

// A centred vertical stack of buttons (pause menu, death and complete screens).
std::vector<Button> stackButtons(const std::vector<std::string>& labels, int cx, int top, int w, int h, int gap,
                                 Color accent);

// Dim the whole frame and draw a panel (used under every overlay).
void drawOverlayPanel(Framebuffer& fb, int x, int y, int w, int h, Color edge);
