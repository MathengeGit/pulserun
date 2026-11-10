// SDL2 backend. SDL is used only to open a window, read input and show a block of
// pixels: we scale the Framebuffer ourselves (nearest neighbour, letterboxed) into a
// software surface and hand it to SDL_UpdateWindowSurface. No SDL drawing calls,
// no GPU renderer.
#if defined(PULSERUN_SDL) && !defined(__EMSCRIPTEN__)
#include "core/window.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <vector>

struct Window::Impl {
    SDL_Window* win = nullptr;
    SDL_Surface* staging = nullptr;  // ARGB8888, window-sized
    int ox = 0, oy = 0, dw = 0, dh = 0;  // where the framebuffer lands inside the window
    int fbW = 0, fbH = 0;
    int keysDown = 0;  // jump keys currently held
    bool mouseDown = false;
    std::vector<int> xmap;
};

Window::Window(int w, int h, const char* title) : impl_(new Impl) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return;
    impl_->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
}

Window::~Window() {
    if (impl_->staging) SDL_FreeSurface(impl_->staging);
    if (impl_->win) SDL_DestroyWindow(impl_->win);
    SDL_Quit();
    delete impl_;
}

bool Window::ok() const { return impl_->win != nullptr; }
bool Window::is_headless() { return false; }

double Window::now_seconds() const {
    return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

static bool isJumpKey(SDL_Keycode k) { return k == SDLK_SPACE || k == SDLK_UP || k == SDLK_w; }

bool Window::poll_events() {
    Impl& im = *impl_;
    auto toFb = [&](int wx, int wy) {
        if (im.dw <= 0 || im.dh <= 0) return;
        input_.pointerX = (wx - im.ox) * im.fbW / im.dw;
        input_.pointerY = (wy - im.oy) * im.fbH / im.dh;
    };
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: return false;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                input_.focusLost = true;
                im.keysDown = 0;
                im.mouseDown = false;
            }
            break;
        case SDL_KEYDOWN: {
            const SDL_Keycode k = e.key.keysym.sym;
            if (e.key.repeat) break;
            if (isJumpKey(k)) { ++im.keysDown; input_.jumpPressed = true; }
            if (k == SDLK_ESCAPE || k == SDLK_p) input_.escPressed = true;
            if (k == SDLK_RETURN || k == SDLK_KP_ENTER) input_.enterPressed = true;
            if (k == SDLK_UP) input_.upPressed = true;
            if (k == SDLK_DOWN) input_.downPressed = true;
            if (k == SDLK_LEFT) input_.leftPressed = true;
            if (k == SDLK_RIGHT) input_.rightPressed = true;
            if (k == SDLK_r) input_.restartPressed = true;
            if (k == SDLK_h) input_.hitboxPressed = true;
            break;
        }
        case SDL_KEYUP:
            if (isJumpKey(e.key.keysym.sym) && im.keysDown > 0) --im.keysDown;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button == SDL_BUTTON_LEFT) {
                im.mouseDown = true;
                input_.jumpPressed = true;
                input_.clicked = true;
                toFb(e.button.x, e.button.y);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT) im.mouseDown = false;
            break;
        case SDL_MOUSEMOTION: toFb(e.motion.x, e.motion.y); break;
        default: break;
        }
    }
    input_.jumpHeld = im.keysDown > 0 || im.mouseDown;
    return true;
}

void Window::present(const Framebuffer& fb) {
    Impl& im = *impl_;
    if (!im.win) return;
    SDL_Surface* screen = SDL_GetWindowSurface(im.win);
    if (!screen) return;
    const int W = screen->w, H = screen->h;
    if (!im.staging || im.staging->w != W || im.staging->h != H) {
        if (im.staging) SDL_FreeSurface(im.staging);
        im.staging = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
        if (!im.staging) return;
    }
    // Letterbox: largest size with the framebuffer's aspect ratio that fits.
    im.fbW = fb.width();
    im.fbH = fb.height();
    im.dw = std::min(W, H * im.fbW / im.fbH);
    im.dh = im.dw * im.fbH / im.fbW;
    im.ox = (W - im.dw) / 2;
    im.oy = (H - im.dh) / 2;

    im.xmap.resize(im.dw);
    for (int x = 0; x < im.dw; ++x) im.xmap[x] = x * im.fbW / im.dw;

    SDL_LockSurface(im.staging);
    auto* base = static_cast<uint8_t*>(im.staging->pixels);
    const Color* src = fb.data();
    for (int y = 0; y < H; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(base + y * im.staging->pitch);
        if (y < im.oy || y >= im.oy + im.dh) {
            std::fill(row, row + W, 0xFF000000u);
            continue;
        }
        const Color* srow = src + (size_t)((y - im.oy) * im.fbH / im.dh) * im.fbW;
        std::fill(row, row + im.ox, 0xFF000000u);
        for (int x = 0; x < im.dw; ++x) {
            const Color c = srow[im.xmap[x]];
            row[im.ox + x] = 0xFF000000u | (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
        }
        std::fill(row + im.ox + im.dw, row + W, 0xFF000000u);
    }
    SDL_UnlockSurface(im.staging);
    SDL_BlitSurface(im.staging, nullptr, screen, nullptr);
    SDL_UpdateWindowSurface(im.win);
}
#endif
