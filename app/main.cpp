// PulseRun entry point: a 1280x720 window showing a 960x540 framebuffer, driven by
// the fixed-timestep GameLoop. Headless builds (no SDL2) write a PNG instead.
#include "core/window.h"
#include "game/camera.h"
#include "game/game_loop.h"
#include "game/sprites.h"
#include "ui/ui.h"
#include <cmath>
#include <cstdio>

namespace {
// Scrolling scene: obstacles live in world space and reach the screen only through the
// camera's world-to-screen matrix.
class DemoScene : public GameState {
public:
    DemoScene() {
        cam_.reset(Vec2(0, kGround - 28), kGround);
        prevCam_ = cam_;
    }
    void update(float dt, const Input&) override {
        prevCam_ = cam_;
        playerX_ += 300.0f * dt;
        time_ += dt;
        cam_.follow(Vec2(playerX_, kGround - 28), kGround, dt);
    }
    void render(Framebuffer& fb, float alpha) override {
        const Camera cam = Camera::lerp(prevCam_, cam_, alpha);
        const Mat3 view = cam.world_to_screen();
        const float groundScreen = (view * Vec2(0, kGround)).y;
        drawBackdrop(fb, cam.scroll_x, (int)groundScreen, pal_);
        drawGround(fb, (int)groundScreen, cam.scroll_x, pal_);
        // A repeating strip of world objects every 900 px.
        const int first = (int)std::floor(cam.scroll_x / 900.0f);
        for (int k = first; k <= first + 2; ++k) {
            const float x = k * 900.0f;
            const Vec2 spike = view * Vec2(x + 300, kGround);
            drawSpike(fb, spike.x, spike.y, 50, 50, pal_);
            const Vec2 block = view * Vec2(x + 520, kGround - 60);
            drawBlock(fb, block.x, block.y, 60, pal_);
            drawSaw(fb, view * Vec2(x + 760, kGround - 90), 40, time_ * 4.0f, pal_);
        }
        drawCube(fb, view * Vec2(playerX_, kGround - 28), 56, 0.0f, pal_);
        drawHud(fb, 1, 0.0f, 1, pal_);
    }

private:
    static constexpr float kGround = 500.0f;
    Palette pal_ = defaultPalette();
    Camera cam_, prevCam_;
    float playerX_ = 0, time_ = 0;
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
