// Headless backend: no window at all. Used in CI and when SDL2 is not installed.
// The app checks Window::ok() and falls back to rendering PNG files.
#if !defined(PULSERUN_SDL) && !defined(__EMSCRIPTEN__)
#include "core/window.h"
#include <chrono>

struct Window::Impl {};

Window::Window(int, int, const char*) {}
Window::~Window() = default;
bool Window::ok() const { return false; }
bool Window::poll_events() { return false; }
void Window::present(const Framebuffer&) {}
bool Window::is_headless() { return true; }

double Window::now_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
#endif
