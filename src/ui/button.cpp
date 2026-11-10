#include "ui/button.h"
#include "raster/font.h"
#include "ui/ui.h"

void drawButton(Framebuffer& fb, const Button& b, bool focused) {
    const Color edge = b.enabled ? b.accent : rgb(90, 92, 130);
    if (focused && b.enabled) {
        fillRoundedRect(fb, b.x - 3, b.y - 3, b.w + 6, b.h + 6, 13, withAlpha(edge, 40));
        drawRoundedRect(fb, b.x - 7, b.y - 7, b.w + 14, b.h + 14, 16, rgb(245, 248, 255), 2);
    }
    fillRoundedRect(fb, b.x, b.y, b.w, b.h, 10, focused ? rgb(22, 26, 70) : rgb(14, 16, 48));
    drawRoundedRect(fb, b.x, b.y, b.w, b.h, 10, edge, 3);
    const int scale = b.h >= 50 ? 3 : 2;
    drawTextCentered(fb, b.x + b.w / 2, b.y + (b.h - 7 * scale) / 2, b.label, scale,
                     b.enabled ? rgb(240, 244, 255) : rgb(120, 122, 160));
}

int buttonAt(const std::vector<Button>& buttons, int px, int py) {
    for (size_t i = 0; i < buttons.size(); ++i)
        if (buttons[i].enabled && buttons[i].contains(px, py)) return (int)i;
    return -1;
}

int moveFocus(const std::vector<Button>& buttons, int focus, int delta) {
    const int n = (int)buttons.size();
    if (n == 0) return -1;
    for (int k = 1; k <= n; ++k) {
        const int i = ((focus + delta * k) % n + n) % n;
        if (buttons[i].enabled) return i;
    }
    return focus;
}

std::vector<Button> stackButtons(const std::vector<std::string>& labels, int cx, int top, int w, int h, int gap,
                                 Color accent) {
    std::vector<Button> out;
    for (size_t i = 0; i < labels.size(); ++i) {
        Button b;
        b.x = cx - w / 2;
        b.y = top + (int)i * (h + gap);
        b.w = w;
        b.h = h;
        b.label = labels[i];
        b.accent = accent;
        out.push_back(b);
    }
    return out;
}

void drawOverlayPanel(Framebuffer& fb, int x, int y, int w, int h, Color edge) {
    fb.fillRect(0, 0, fb.width(), fb.height(), rgba(6, 7, 24, 150));
    for (int i = 3; i >= 1; --i) fillRoundedRect(fb, x - i * 3, y - i * 3, w + i * 6, h + i * 6, 18 + i * 3, withAlpha(edge, 14));
    fillRoundedRect(fb, x, y, w, h, 18, rgba(12, 14, 44, 240));
    drawRoundedRect(fb, x, y, w, h, 18, edge, 3);
}
