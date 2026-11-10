// PulseRun entry point: opens a 1280x720 window and shows a 960x540 framebuffer
// every frame. Headless builds (no SDL2) write a PNG instead.
#include "core/window.h"
#include "game/sprites.h"
#include "ui/ui.h"
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
const int W = 960, H = 540;

struct App {
    Window win{1280, 720, "PulseRun"};
    Framebuffer fb{W, H};
    Palette pal = defaultPalette();
    double start = win.now_seconds();

    void drawFrame() {
        const float t = (float)(win.now_seconds() - start);
        const float scroll = t * 240.0f;
        drawBackdrop(fb, scroll, 440, pal);
        drawGround(fb, 440, scroll, pal);
        drawSaw(fb, Vec2(700, 358), 44, t * 4.0f, pal);
        drawCube(fb, Vec2(240, 412), 56, 0.0f, pal);
        drawHud(fb, 1, 0.0f, 1, pal);
    }
};

#ifdef __EMSCRIPTEN__
void webFrame(void* arg) {
    App& app = *static_cast<App*>(arg);
    app.win.poll_events();
    app.drawFrame();
    app.win.present(app.fb);
}
#endif
}  // namespace

int main(int argc, char** argv) {
    static App app;
    if (!app.win.ok()) {
        const char* out = argc > 1 ? argv[1] : "pulserun_frame.png";
        app.drawFrame();
        std::printf("Headless build: wrote %s\n", out);
        return app.fb.savePNG(out) ? 0 : 1;
    }
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(webFrame, &app, 0, true);
#else
    while (app.win.poll_events()) {
        app.drawFrame();
        app.win.present(app.fb);
    }
#endif
    return 0;
}
