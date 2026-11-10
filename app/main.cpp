// PulseRun entry point: a 1280x720 window showing a 960x540 framebuffer, driven by
// the fixed-timestep GameLoop. Headless builds (no SDL2) write a PNG instead.
#include "core/window.h"
#include "game/game_loop.h"
#include "game/sprites.h"
#include "ui/ui.h"
#include <cstdio>

namespace {
// Scrolling scene: the scroll advances in fixed steps and is interpolated for rendering.
class DemoScene : public GameState {
public:
    void update(float dt, const Input&) override {
        prevScroll_ = scroll_;
        scroll_ += 240.0f * dt;
        time_ += dt;
    }
    void render(Framebuffer& fb, float alpha) override {
        const float s = prevScroll_ + (scroll_ - prevScroll_) * alpha;
        drawBackdrop(fb, s, 440, pal_);
        drawGround(fb, 440, s, pal_);
        drawSaw(fb, Vec2(700, 358), 44, time_ * 4.0f, pal_);
        drawCube(fb, Vec2(240, 412), 56, 0.0f, pal_);
        drawHud(fb, 1, 0.0f, 1, pal_);
    }

private:
    Palette pal_ = defaultPalette();
    float scroll_ = 0, prevScroll_ = 0, time_ = 0;
};
}  // namespace

int main(int argc, char** argv) {
    static Window win(1280, 720, "PulseRun");
    static GameLoop loop;
    static DemoScene scene;
    if (!win.ok()) {
        const char* out = argc > 1 ? argv[1] : "pulserun_frame.png";
        loop.advance(1.0, scene, Input{});
        scene.render(loop.framebuffer(), 0.0f);
        std::printf("Headless build: wrote %s\n", out);
        return loop.framebuffer().savePNG(out) ? 0 : 1;
    }
    loop.run(win, scene);
    return 0;
}
