#include "game/game_loop.h"
#include <algorithm>
#include <chrono>
#include <thread>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

GameLoop::GameLoop(int fbWidth, int fbHeight) : fb_(fbWidth, fbHeight) {}

int GameLoop::advance(double realDt, GameState& state, const Input& in) {
    pending_.merge(in);
    accumulator_ += std::clamp(realDt, 0.0, MAX_FRAME);
    int n = 0;
    while (accumulator_ >= FIXED_DT) {
        state.update((float)FIXED_DT, pending_);
        pending_.clearEdges();  // each edge is seen by exactly one update
        accumulator_ -= FIXED_DT;
        ++steps_;
        ++n;
    }
    return n;
}

bool GameLoop::frame(Window& win, GameState& state) {
    if (!win.poll_events()) return false;
    const Input in = win.input();
    win.input().clearEdges();
    const double now = win.now_seconds();
    const double dt = last_ < 0 ? FIXED_DT : now - last_;
    last_ = now;
    advance(dt, state, in);
    state.render(fb_, alpha());
    win.present(fb_);
    return !state.wantsQuit();
}

#ifdef __EMSCRIPTEN__
namespace {
struct WebCtx {
    GameLoop* loop;
    Window* win;
    GameState* state;
};
void webFrame(void* p) {
    auto* c = static_cast<WebCtx*>(p);
    c->loop->frame(*c->win, *c->state);
}
}  // namespace
#endif

void GameLoop::run(Window& win, GameState& state) {
    if (!win.ok()) return;
#ifdef __EMSCRIPTEN__
    static WebCtx ctx;
    ctx = WebCtx{this, &win, &state};
    // The browser calls webFrame once per display refresh (requestAnimationFrame).
    emscripten_set_main_loop_arg(webFrame, &ctx, 0, true);
#else
    while (frame(win, state)) {
        // No vsync on a software surface: sleep off most of the time until the next step.
        const double spare = FIXED_DT - accumulator_;
        if (spare > 0.004) std::this_thread::sleep_for(std::chrono::microseconds((int)((spare - 0.002) * 1e6)));
    }
#endif
}
