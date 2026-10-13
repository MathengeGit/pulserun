#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Color {
    uint8_t r, g, b, a;
};

class Framebuffer {
public:
    Framebuffer(int width, int height);

    void clear(Color c);
    void setPixel(int x, int y, Color c);   // ignores out-of-bounds writes
    bool saveBMP(const std::string& path) const;

    int width() const { return w_; }
    int height() const { return h_; }

private:
    int w_, h_;
    std::vector<Color> px_;
};