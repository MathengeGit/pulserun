#include "core/framebuffer.h"
#include <fstream>

Framebuffer::Framebuffer(int width, int height)
    : w_(width), h_(height), px_(width * height, Color{0, 0, 0, 255}) {}

void Framebuffer::clear(Color c) {
    for (auto& p : px_) p = c;
}

void Framebuffer::setPixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    px_[y * w_ + x] = c;
}

bool Framebuffer::saveBMP(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    const int rowSize = (3 * w_ + 3) & ~3;          // rows padded to 4 bytes
    const uint32_t dataSize = rowSize * h_;
    uint8_t header[54] = {0};
    header[0] = 'B'; header[1] = 'M';

    auto put32 = [&](int off, uint32_t v) {
        for (int i = 0; i < 4; ++i) header[off + i] = (v >> (8 * i)) & 0xFF;
    };
    put32(2, 54 + dataSize);   // file size
    put32(10, 54);             // pixel data offset
    put32(14, 40);             // DIB header size
    put32(18, w_);
    put32(22, h_);
    header[26] = 1;            // colour planes
    header[28] = 24;           // bits per pixel
    put32(34, dataSize);
    f.write(reinterpret_cast<char*>(header), 54);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = h_ - 1; y >= 0; --y) {                 // BMP stores bottom row first
        for (int x = 0; x < w_; ++x) {
            const Color& c = px_[y * w_ + x];
            row[3 * x] = c.b; row[3 * x + 1] = c.g; row[3 * x + 2] = c.r;
        }
        f.write(reinterpret_cast<char*>(row.data()), rowSize);
    }
    return true;
}