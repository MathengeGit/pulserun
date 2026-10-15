#pragma once
#include "core/framebuffer.h"
#include "raster/font.h"
#include <iostream>
#include <string>

// Every demo takes an optional output folder as its first argument, e.g.
//   demo_lines docs/screenshots
inline std::string outPath(int argc, char** argv, const char* name) {
    std::string dir = (argc > 1) ? argv[1] : ".";
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += '/';
    return dir + name;
}

inline int saveDemo(const Framebuffer& fb, int argc, char** argv, const char* name) {
    const std::string path = outPath(argc, argv, name);
    if (!fb.savePNG(path)) {
        std::cerr << "FAILED to write " << path << "\n";
        return 1;
    }
    std::cout << "Wrote " << path << "\n";
    return 0;
}

// Shared look for the technique demos.
inline Color demoBg() { return rgb(12, 14, 34); }
inline Color demoText() { return rgb(235, 240, 255); }
inline Color demoDim() { return rgb(140, 150, 195); }
inline Color demoFrame() { return rgb(56, 64, 120); }
