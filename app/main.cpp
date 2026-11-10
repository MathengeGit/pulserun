// PulseRun entry point.
//   pulserun                      play (SDL2 window on desktop, <canvas> in the browser)
//   pulserun --showcase <dir>     render every screen to PNG, no window needed (used by CI)
//   pulserun --help
// A headless build (no SDL2) has no window, so it always renders the showcase.
#include "core/storage.h"
#include "core/window.h"
#include "game/game.h"
#include "game/game_loop.h"
#include "game/solver.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {

// levels/ next to the working directory, the source tree (desktop), or the embedded
// /levels (browser build).
std::string findLevelDir() {
    for (const std::string& d : {std::string("levels"), std::string("../levels"),
                                 std::string(PULSERUN_SOURCE_DIR) + "/levels", std::string("/levels")}) {
        std::ifstream probe(d + "/level1.txt");
        if (probe) return d;
    }
    return "levels";
}

std::vector<Level> loadLevels() {
    std::vector<Level> out;
    const std::string dir = findLevelDir();
    for (int n = 1;; ++n) {
        Level l;
        std::string err;
        if (!loadLevel(dir + "/level" + std::to_string(n) + ".txt", l, &err)) {
            if (n == 1) std::fprintf(stderr, "No levels: %s\n", err.c_str());
            break;
        }
        out.push_back(l);
    }
    return out;
}

// ---------- Headless showcase: drives the real Game and saves each screen ----------

struct Showcase {
    Game& game;
    Framebuffer fb{Game::W, Game::H};
    std::string dir;
    int failures = 0;

    void step(const Input& in = Input{}, int n = 1) {
        for (int i = 0; i < n; ++i) game.update((float)GameLoop::FIXED_DT, in);
    }
    void shot(const char* name) {
        game.render(fb, 0.0f);
        const std::string path = dir + "/" + name;
        if (fb.savePNG(path)) std::printf("Wrote %s\n", path.c_str());
        else ++failures;
    }
    // Play a level with the solver's inputs for `fraction` of the plan.
    void play(int level, const std::vector<uint8_t>& plan, float fraction) {
        game.startLevel(level);
        bool prev = false;
        const size_t n = (size_t)(plan.size() * fraction);
        for (size_t i = 0; i < n && game.screen() == Screen::Playing; ++i) {
            Input in;
            in.jumpHeld = plan[i] != 0;
            in.jumpPressed = in.jumpHeld && !prev;
            prev = in.jumpHeld;
            step(in);
        }
    }
};

int runShowcase(const std::vector<Level>& levels, const std::string& dir) {
    setStorageEnabled(false);
    if (levels.empty()) return 1;
    Game game(levels, false);
    Showcase s{game, Framebuffer(Game::W, Game::H), dir};
    s.step(Input{}, 40);
    s.shot("10_game_menu.png");

    game.showScreen(Screen::LevelSelect);
    Input right;
    right.rightPressed = true;
    s.step(right);
    s.step(Input{}, 20);
    s.shot("11_game_level_select.png");

    game.showScreen(Screen::Settings);
    s.step(Input{}, 10);
    s.shot("12_game_settings.png");

    const SolveResult plan1 = solveLevel(levels[0]);
    s.play(0, plan1.held, 0.43f);
    s.shot("13_game_play.png");
    game.setHitboxes(true);
    s.shot("14_game_hitboxes.png");
    game.setHitboxes(false);

    Input esc;
    esc.escPressed = true;
    s.step(esc);
    s.shot("15_game_paused.png");

    const int deathLevel = levels.size() > 1 ? 1 : 0;
    game.startLevel(deathLevel);
    for (int i = 0; i < 2000 && game.screen() == Screen::Playing; ++i) s.step();
    s.step(Input{}, 3);
    s.shot("16_game_death_burst.png");
    for (int i = 0; i < 120 && game.screen() != Screen::Dead; ++i) s.step();
    s.step(Input{}, 15);
    s.shot("17_game_crashed.png");

    s.play(0, plan1.held, 1.0f);
    for (int i = 0; i < 600 && game.screen() == Screen::Playing; ++i) s.step();
    s.step(Input{}, 40);
    s.shot("18_game_complete.png");
    if (game.screen() != Screen::Complete) {
        std::fprintf(stderr, "Showcase: level 1 plan did not reach the complete screen\n");
        ++s.failures;
    }
    return s.failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    std::string showcaseDir;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--help")) {
            std::printf("pulserun                   play\n"
                        "pulserun --showcase <dir>  render every screen to PNG files in <dir>\n");
            return 0;
        }
        if (!std::strcmp(argv[i], "--showcase") && i + 1 < argc) showcaseDir = argv[++i];
    }
    static std::vector<Level> levels = loadLevels();
    if (!showcaseDir.empty()) return runShowcase(levels, showcaseDir);

    static Window win(1280, 720, "PulseRun");
    if (!win.ok()) {
        std::printf("Headless build (no window): rendering the showcase into the current folder.\n");
        return runShowcase(levels, ".");
    }
    static GameLoop loop;
    static Game game(levels, true);
    loop.run(win, game);
    return 0;
}
