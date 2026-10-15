#pragma once
#include "core/color.h"
#include <string>
#include <vector>

// A canvas of pixels in memory. (0,0) is the top-left corner, y grows downward.
// Every raster algorithm in this project draws through this class, so the
// "graphics API" is just setPixel/blendPixel. No external library is used.
class Framebuffer {
public:
    Framebuffer(int width, int height);

    void clear(Color c);
    void setPixel(int x, int y, Color c);    // overwrite, ignores alpha
    void blendPixel(int x, int y, Color c);  // source-over alpha blend
    Color getPixel(int x, int y) const;

    void hLine(int x0, int x1, int y, Color c);           // blended horizontal span
    void fillRect(int x, int y, int w, int h, Color c);   // blended axis-aligned rectangle

    bool saveBMP(const std::string& path) const;  // 24-bit BMP, hand-written encoder
    bool savePNG(const std::string& path) const;  // PNG with stored (uncompressed) deflate blocks

    int width() const { return w_; }
    int height() const { return h_; }

private:
    int w_, h_;
    std::vector<Color> px_;
};
