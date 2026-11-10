// PulseRun entry point: a 1280x720 window showing a 960x540 framebuffer, driven by
// the fixed-timestep GameLoop. Headless builds (no SDL2) write a PNG instead.
#include "core/window.h"
#include "game/camera.h"
#include "game/game_loop.h"
#include "game/collision.h"
#include "game/player.h"
#include "game/sprites.h"
#include "ui/ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
// Scrolling scene: obstacles live in world space and reach the screen only through the
// camera's world-to-screen matrix.
class DemoScene : public GameState {
public:
    DemoScene() {
        for (int k = 1; k <= 40; ++k) {  // a repeating strip of obstacles every 900 px
            const float x = k * 900.0f;
            obs_.push_back(make(ObstacleType::Spike, x + 300, kGround - 50, 50, 50, 0));
            obs_.push_back(make(ObstacleType::Block, x + 520, kGround - 60, 60, 60, 0));
            obs_.push_back(make(ObstacleType::Saw, x + 760, kGround - 150, 0, 0, 36));
        }
        player_.reset(Vec2(0, kGround - Player::SIZE / 2), 300.0f);
        cam_.reset(player_.pos, kGround);
        prevCam_ = cam_;
    }
    // Space / click jumps (hold for higher). Spikes and saws kill, blocks can be stood on.
    void update(float dt, const Input& in) override {
        prevCam_ = cam_;
        time_ += dt;
        player_.setHeld(in.jumpHeld);
        player_.update(dt);
        const CollisionResult r = resolve(player_, obs_, kGround, time_);
        if (r.dead) {
            player_.reset(Vec2(0, kGround - Player::SIZE / 2), 300.0f);
            cam_.reset(player_.pos, kGround);
            prevCam_ = cam_;
            ++attempt_;
            return;
        }
        if (player_.on_ground && (in.jumpPressed || in.jumpHeld)) player_.jump();
        cam_.follow(player_.pos, kGround, dt);
    }
    void render(Framebuffer& fb, float alpha) override {
        const Camera cam = Camera::lerp(prevCam_, cam_, alpha);
        const Mat3 view = cam.world_to_screen();
        const float groundScreen = (view * Vec2(0, kGround)).y;
        drawBackdrop(fb, cam.scroll_x, (int)groundScreen, pal_);
        drawGround(fb, (int)groundScreen, cam.scroll_x, pal_);
        for (const Obstacle& o : obs_) {
            const Vec2 s = view * Vec2(o.x, o.y);
            if (s.x < -120 || s.x > fb.width() + 120) continue;
            if (o.type == ObstacleType::Spike) drawSpike(fb, s.x + o.w / 2, s.y + o.h, o.w, o.h, pal_);
            if (o.type == ObstacleType::Block) drawBlock(fb, s.x, s.y, o.w, pal_);
            if (o.type == ObstacleType::Saw) drawSaw(fb, s, o.radius, o.sawAngle(time_), pal_);
        }
        const Vec2 p = player_.prev + (player_.pos - player_.prev) * alpha;
        drawCube(fb, view * p, 56, player_.prevAngle + (player_.angle - player_.prevAngle) * alpha, pal_);
        drawHud(fb, 1, std::min(1.0f, player_.pos.x / 36000.0f), attempt_, pal_);
    }

private:
    static constexpr float kGround = 500.0f;
    Palette pal_ = defaultPalette();
    Camera cam_, prevCam_;
    static Obstacle make(ObstacleType t, float x, float y, float w, float h, float r) {
        Obstacle o;
        o.type = t;
        o.x = x; o.y = y; o.w = w; o.h = h; o.radius = r; o.speed = 0.6f;
        return o;
    }
    std::vector<Obstacle> obs_;
    Player player_;
    int attempt_ = 1;
    float time_ = 0;
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
