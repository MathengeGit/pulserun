// PulseRun entry point: a 1280x720 window showing a 960x540 framebuffer, driven by
// the fixed-timestep GameLoop. Headless builds (no SDL2) write a PNG instead.
#include "core/window.h"
#include "game/game_loop.h"
#include "game/level.h"
#include "game/world_render.h"
#include "raster/font.h"
#include "ui/ui.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

// levels/ next to the working directory, the source tree (desktop), or the embedded
// /levels (browser build).
std::string findLevelDir() {
    for (const std::string& d : {std::string("levels"), std::string("../levels"), std::string(PULSERUN_SOURCE_DIR) + "/levels",
                                std::string("/levels")}) {
        std::ifstream probe(d + "/level1.txt");
        if (probe) return d;
    }
    return "levels";
}

// Plays the levels in order: death restarts the attempt, the portal moves to the next level.
class PlayScene : public GameState {
public:
    PlayScene() {
        const std::string dir = findLevelDir();
        for (int n = 1;; ++n) {
            Level l;
            std::string err;
            if (!loadLevel(dir + "/level" + std::to_string(n) + ".txt", l, &err)) break;
            levels_.push_back(l);
        }
        if (!levels_.empty()) start(0);
    }

    void update(float dt, const Input& in) override {
        if (levels_.empty()) return;
        prevCam_ = cam_;
        if (in.hitboxPressed) opt_.hitboxes = !opt_.hitboxes;
        if (in.restartPressed) restart();
        if (world_.dead || world_.complete) {
            pause_ += dt;
            if (pause_ > 0.6f) {
                if (world_.complete) start((index_ + 1) % (int)levels_.size());
                else restart();
            }
            return;
        }
        world_.step(dt, in.jumpHeld, in.jumpPressed);
        cam_.follow(world_.player.pos, world_.level->groundY, dt);
    }

    void render(Framebuffer& fb, float alpha) override {
        if (levels_.empty()) {
            fb.clear(rgb(10, 11, 36));
            drawTextCentered(fb, fb.width() / 2, 260, "NO LEVELS FOUND", 4, rgb(255, 90, 120));
            return;
        }
        const Camera cam = Camera::lerp(prevCam_, cam_, alpha);
        drawWorld(fb, world_, cam, alpha, pal_, opt_);
        if (world_.dead) drawDeathBurst(fb, cam.toScreen(world_.player.pos), std::min(1.0f, pause_ * 1.6f), pal_,
                                        ClipRect{0, 0, (double)fb.width() - 1, (double)fb.height() - 1});
        drawHud(fb, world_.level->number, world_.progress(), attempt_, pal_);
    }

private:
    void start(int i) {
        index_ = i;
        attempt_ = 1;
        world_.setLevel(&levels_[i]);
        pal_ = levelPalette(levels_[i]);
        restart();
        attempt_ = 1;
    }
    void restart() {
        world_.reset();
        cam_.reset(world_.player.pos, world_.level->groundY);
        prevCam_ = cam_;
        pause_ = 0;
        ++attempt_;
    }

    std::vector<Level> levels_;
    World world_;
    Camera cam_, prevCam_;
    Palette pal_ = defaultPalette();
    RenderOptions opt_;
    int index_ = 0, attempt_ = 1;
    float pause_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    static Window win(1280, 720, "PulseRun");
    static GameLoop loop;
    static PlayScene scene;
    if (!win.ok()) {
        const char* out = argc > 1 ? argv[1] : "pulserun_frame.png";
        loop.advance(2.1, scene, Input{});
        scene.render(loop.framebuffer(), 0.0f);
        std::printf("Headless build: wrote %s\n", out);
        return loop.framebuffer().savePNG(out) ? 0 : 1;
    }
    loop.run(win, scene);
    return 0;
}
