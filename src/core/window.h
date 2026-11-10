#pragma once
#include "core/framebuffer.h"
#include "core/input.h"

// A real-time window that shows a Framebuffer. It only copies finished pixels to
// the screen; every pixel is still produced by our own raster code.
//
// Three backends, chosen at compile time:
//   - SDL2       (desktop, when CMake finds SDL2 and PULSERUN_HEADLESS is off): window_sdl.cpp
//   - Emscripten (browser build): blits into an HTML <canvas id="canvas">:      window_web.cpp
//   - headless   (CI / no SDL2): ok() is false; the app renders PNGs instead:     window_headless.cpp
class Window {
public:
    Window(int w, int h, const char* title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ok() const;      // false if no window could be opened (headless build)
    bool poll_events();   // pumps OS events into input(); returns false once the window is closed
    void present(const Framebuffer& fb);  // scales fb to the window (letterboxed, nearest neighbour)

    Input& input() { return input_; }
    double now_seconds() const;  // monotonic clock

    static bool is_headless();

private:
    struct Impl;
    Impl* impl_ = nullptr;
    Input input_;
};
