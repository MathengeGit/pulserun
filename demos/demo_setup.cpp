#include "core/framebuffer.h"
#include <iostream>

int main() {
    Framebuffer fb(960, 540);
    fb.clear({20, 20, 40, 255});                        // dark navy background

    for (int x = 0; x < fb.width(); ++x)                // ground line
        fb.setPixel(x, 440, {0, 255, 200, 255});

    for (int y = 380; y < 440; ++y)                     // placeholder cube
        for (int x = 200; x < 260; ++x)
            fb.setPixel(x, y, {80, 255, 80, 255});

    if (!fb.saveBMP("setup.bmp")) {
        std::cerr << "Failed to write setup.bmp\n";
        return 1;
    }
    std::cout << "Wrote setup.bmp\n";
    return 0;
}