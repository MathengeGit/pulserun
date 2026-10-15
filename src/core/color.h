#pragma once
#include <algorithm>
#include <cstdint>

// 8-bit RGBA colour. Alpha is only used by Framebuffer::blendPixel.
struct Color {
    uint8_t r, g, b, a;
};

inline Color rgba(int r, int g, int b, int a) {
    return Color{(uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255),
                 (uint8_t)std::clamp(b, 0, 255), (uint8_t)std::clamp(a, 0, 255)};
}
inline Color rgb(int r, int g, int b) { return rgba(r, g, b, 255); }

inline Color withAlpha(Color c, int a) {
    c.a = (uint8_t)std::clamp(a, 0, 255);
    return c;
}

// Linear interpolation between two colours (t in [0,1]).
inline Color lerpColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto m = [&](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * t + 0.5f); };
    return Color{m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

// Multiply RGB by k (k<1 darkens).
inline Color shade(Color c, float k) {
    return rgba((int)(c.r * k), (int)(c.g * k), (int)(c.b * k), c.a);
}
