// Browser backend (Emscripten). The finished Framebuffer is copied into an HTML
// <canvas id="canvas"> with putImageData: our RGBA bytes go straight to the page,
// so the browser does no drawing of its own. Input comes from html5.h callbacks.
#ifdef __EMSCRIPTEN__
#include "core/window.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <cstring>

// clang-format off
EM_JS(void, pr_js_present, (const void* px, int w, int h), {
    const c = Module['canvas'] || document.getElementById('canvas');
    if (!c) return;
    if (c.width !== w) c.width = w;
    if (c.height !== h) c.height = h;
    if (!Module.__prCtx) Module.__prCtx = c.getContext('2d', { alpha: false });
    const img = new ImageData(new Uint8ClampedArray(HEAPU8.buffer, px, w * h * 4), w, h);
    Module.__prCtx.putImageData(img, 0, 0);
});

// The page sets Module.prKeysActive = false while the game is scrolled out of view,
// so Space and the arrow keys scroll the page instead of being captured.
EM_JS(int, pr_js_keys_active, (), {
    return (Module['prKeysActive'] === false) ? 0 : 1;
});
// clang-format on

struct Window::Impl {
    int keysDown = 0;
    bool pointerDown = false;
    int fbW = 960, fbH = 540;
};

namespace {
struct Ctx {
    Window::Impl* im;
    Input* in;
};
Ctx g_ctx{nullptr, nullptr};

bool is(const EmscriptenKeyboardEvent* e, const char* code) { return std::strcmp(e->code, code) == 0; }
bool isJump(const EmscriptenKeyboardEvent* e) { return is(e, "Space") || is(e, "ArrowUp") || is(e, "KeyW"); }

void toFb(long tx, long ty) {
    double cw = 0, ch = 0;
    emscripten_get_element_css_size("#canvas", &cw, &ch);
    if (cw <= 0 || ch <= 0) return;
    g_ctx.in->pointerX = (int)(tx * g_ctx.im->fbW / cw);
    g_ctx.in->pointerY = (int)(ty * g_ctx.im->fbH / ch);
}

EM_BOOL onKeyDown(int, const EmscriptenKeyboardEvent* e, void*) {
    if (!pr_js_keys_active()) return EM_FALSE;
    Input& in = *g_ctx.in;
    bool handled = true;
    if (isJump(e)) {
        if (!e->repeat) { ++g_ctx.im->keysDown; in.jumpPressed = true; }
    } else if (is(e, "Escape") || is(e, "KeyP")) {
        in.escPressed = !e->repeat || in.escPressed;
    } else if (is(e, "Enter") || is(e, "NumpadEnter")) {
        in.enterPressed = true;
    } else if (is(e, "KeyR")) {
        in.restartPressed = true;
    } else if (is(e, "KeyH")) {
        in.hitboxPressed = true;
    } else if (!(is(e, "ArrowDown") || is(e, "ArrowLeft") || is(e, "ArrowRight"))) {
        handled = false;  // let the page handle everything else (e.g. "?" for help)
    }
    if (is(e, "ArrowUp")) in.upPressed = true;
    if (is(e, "ArrowDown")) in.downPressed = true;
    if (is(e, "ArrowLeft")) in.leftPressed = true;
    if (is(e, "ArrowRight")) in.rightPressed = true;
    return handled ? EM_TRUE : EM_FALSE;
}

EM_BOOL onKeyUp(int, const EmscriptenKeyboardEvent* e, void*) {
    if (isJump(e) && g_ctx.im->keysDown > 0) --g_ctx.im->keysDown;
    return EM_FALSE;
}

EM_BOOL onMouseDown(int, const EmscriptenMouseEvent* e, void*) {
    if (e->button != 0) return EM_FALSE;
    g_ctx.im->pointerDown = true;
    g_ctx.in->jumpPressed = true;
    g_ctx.in->clicked = true;
    toFb(e->targetX, e->targetY);
    return EM_TRUE;
}
EM_BOOL onMouseUp(int, const EmscriptenMouseEvent*, void*) {
    g_ctx.im->pointerDown = false;
    return EM_FALSE;
}
EM_BOOL onMouseMove(int, const EmscriptenMouseEvent* e, void*) {
    toFb(e->targetX, e->targetY);
    return EM_FALSE;
}

EM_BOOL onTouchStart(int, const EmscriptenTouchEvent* e, void*) {
    for (int i = 0; i < e->numTouches; ++i)
        if (e->touches[i].isChanged) {
            toFb(e->touches[i].targetX, e->touches[i].targetY);
            break;
        }
    g_ctx.im->pointerDown = true;
    g_ctx.in->jumpPressed = true;
    g_ctx.in->clicked = true;
    return EM_TRUE;  // suppress the emulated mouse events
}
EM_BOOL onTouchEnd(int, const EmscriptenTouchEvent* e, void*) {
    int stillDown = 0;
    for (int i = 0; i < e->numTouches; ++i)
        if (!e->touches[i].isChanged) ++stillDown;
    if (stillDown == 0) g_ctx.im->pointerDown = false;
    return EM_TRUE;
}

EM_BOOL onBlur(int, const EmscriptenFocusEvent*, void*) {
    g_ctx.im->keysDown = 0;
    g_ctx.im->pointerDown = false;
    g_ctx.in->focusLost = true;
    return EM_FALSE;
}
}  // namespace

Window::Window(int, int, const char*) : impl_(new Impl) {
    g_ctx = Ctx{impl_, &input_};
    const char* doc = EMSCRIPTEN_EVENT_TARGET_DOCUMENT;
    emscripten_set_keydown_callback(doc, nullptr, 1, onKeyDown);
    emscripten_set_keyup_callback(doc, nullptr, 1, onKeyUp);
    emscripten_set_mousedown_callback("#canvas", nullptr, 1, onMouseDown);
    emscripten_set_mouseup_callback(doc, nullptr, 1, onMouseUp);
    emscripten_set_mousemove_callback("#canvas", nullptr, 1, onMouseMove);
    emscripten_set_touchstart_callback("#canvas", nullptr, 1, onTouchStart);
    emscripten_set_touchend_callback("#canvas", nullptr, 1, onTouchEnd);
    emscripten_set_touchcancel_callback("#canvas", nullptr, 1, onTouchEnd);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, onBlur);
}

Window::~Window() { delete impl_; }
bool Window::ok() const { return true; }
bool Window::is_headless() { return false; }
double Window::now_seconds() const { return emscripten_get_now() / 1000.0; }

bool Window::poll_events() {
    input_.jumpHeld = impl_->keysDown > 0 || impl_->pointerDown;
    return true;
}

void Window::present(const Framebuffer& fb) {
    impl_->fbW = fb.width();
    impl_->fbH = fb.height();
    pr_js_present(fb.data(), fb.width(), fb.height());
}
#endif
