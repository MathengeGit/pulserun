#include "core/framebuffer.h"
#include <algorithm>
#include <fstream>

Framebuffer::Framebuffer(int width, int height)
    : w_(width), h_(height), px_((size_t)width * height, Color{0, 0, 0, 255}) {}

void Framebuffer::clear(Color c) {
    for (auto& p : px_) p = Color{c.r, c.g, c.b, 255};
}

void Framebuffer::setPixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    px_[(size_t)y * w_ + x] = Color{c.r, c.g, c.b, 255};
}

void Framebuffer::blendPixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_ || c.a == 0) return;
    Color& d = px_[(size_t)y * w_ + x];
    if (c.a == 255) {
        d = c;
        return;
    }
    const int a = c.a;
    d.r = (uint8_t)((c.r * a + d.r * (255 - a) + 127) / 255);
    d.g = (uint8_t)((c.g * a + d.g * (255 - a) + 127) / 255);
    d.b = (uint8_t)((c.b * a + d.b * (255 - a) + 127) / 255);
    d.a = 255;
}

Color Framebuffer::getPixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return Color{0, 0, 0, 0};
    return px_[(size_t)y * w_ + x];
}

void Framebuffer::hLine(int x0, int x1, int y, Color c) {
    if (y < 0 || y >= h_) return;
    if (x0 > x1) std::swap(x0, x1);
    x0 = std::max(x0, 0);
    x1 = std::min(x1, w_ - 1);
    for (int x = x0; x <= x1; ++x) blendPixel(x, y, c);
}

void Framebuffer::fillRect(int x, int y, int w, int h, Color c) {
    for (int j = 0; j < h; ++j) hLine(x, x + w - 1, y + j, c);
}

// ---------------------------------------------------------------- BMP
bool Framebuffer::saveBMP(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    const int rowSize = (3 * w_ + 3) & ~3;  // rows are padded to 4 bytes
    const uint32_t dataSize = (uint32_t)(rowSize * h_);
    uint8_t header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    auto put32 = [&](int off, uint32_t v) {
        for (int i = 0; i < 4; ++i) header[off + i] = (v >> (8 * i)) & 0xFF;
    };
    put32(2, 54 + dataSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, (uint32_t)w_);
    put32(22, (uint32_t)h_);
    header[26] = 1;
    header[28] = 24;
    put32(34, dataSize);
    f.write(reinterpret_cast<char*>(header), 54);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = h_ - 1; y >= 0; --y) {  // BMP stores the bottom row first
        for (int x = 0; x < w_; ++x) {
            const Color& c = px_[(size_t)y * w_ + x];
            row[3 * x] = c.b;
            row[3 * x + 1] = c.g;
            row[3 * x + 2] = c.r;
        }
        f.write(reinterpret_cast<char*>(row.data()), rowSize);
    }
    return true;
}

// ---------------------------------------------------------------- PNG
// A minimal PNG encoder: IHDR + one IDAT + IEND. The pixel data is wrapped in a
// zlib stream made of "stored" (uncompressed) deflate blocks, so no compression
// library is needed. Files are larger than normal PNGs but open everywhere.
namespace {

uint32_t crc32(const uint8_t* data, size_t len) {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        ready = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

uint32_t adler32(const std::vector<uint8_t>& d) {
    uint32_t a = 1, b = 0;
    for (uint8_t byte : d) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

void be32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back((uint8_t)(x >> 24));
    v.push_back((uint8_t)(x >> 16));
    v.push_back((uint8_t)(x >> 8));
    v.push_back((uint8_t)x);
}

void writeChunk(std::ofstream& f, const char type[4], const std::vector<uint8_t>& data) {
    std::vector<uint8_t> lenBytes;
    be32(lenBytes, (uint32_t)data.size());
    f.write(reinterpret_cast<const char*>(lenBytes.data()), 4);

    std::vector<uint8_t> body(type, type + 4);
    body.insert(body.end(), data.begin(), data.end());
    f.write(reinterpret_cast<const char*>(body.data()), (std::streamsize)body.size());

    std::vector<uint8_t> crcBytes;
    be32(crcBytes, crc32(body.data(), body.size()));
    f.write(reinterpret_cast<const char*>(crcBytes.data()), 4);
}

}  // namespace

bool Framebuffer::savePNG(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    f.write(reinterpret_cast<const char*>(sig), 8);

    std::vector<uint8_t> ihdr;
    be32(ihdr, (uint32_t)w_);
    be32(ihdr, (uint32_t)h_);
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // colour type: RGB
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    writeChunk(f, "IHDR", ihdr);

    // Raw scanlines: one filter byte (0 = none) followed by RGB triples.
    std::vector<uint8_t> raw;
    raw.reserve((size_t)h_ * (1 + 3 * w_));
    for (int y = 0; y < h_; ++y) {
        raw.push_back(0);
        for (int x = 0; x < w_; ++x) {
            const Color& c = px_[(size_t)y * w_ + x];
            raw.push_back(c.r);
            raw.push_back(c.g);
            raw.push_back(c.b);
        }
    }

    // zlib wrapper + stored deflate blocks (max 65535 bytes each).
    std::vector<uint8_t> z;
    z.push_back(0x78);
    z.push_back(0x01);
    size_t pos = 0;
    while (true) {
        const size_t n = std::min<size_t>(65535, raw.size() - pos);
        const bool last = (pos + n == raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back((uint8_t)(n & 0xFF));
        z.push_back((uint8_t)(n >> 8));
        z.push_back((uint8_t)(~n & 0xFF));
        z.push_back((uint8_t)((~n >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + (long)pos, raw.begin() + (long)(pos + n));
        pos += n;
        if (last) break;
    }
    be32(z, adler32(raw));
    writeChunk(f, "IDAT", z);
    writeChunk(f, "IEND", {});
    return true;
}
