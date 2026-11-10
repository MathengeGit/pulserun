#pragma once
#include "game/animation.h"
#include "game/camera.h"
#include "game/game_state.h"
#include "game/level.h"
#include "game/progress.h"
#include "game/world.h"
#include "game/world_render.h"
#include "ui/button.h"
#include <vector>

enum class Screen { Menu, LevelSelect, Settings, Playing, Paused, Dying, Dead, Complete };

// The whole game: menus, level select, settings, play, pause, death and level-complete.
// Every screen is keyboard (arrows/Enter/Esc), mouse and touch operable.
class Game : public GameState {
public:
    static constexpr int W = 960, H = 540;

    // `persist` = load/save progress through core/storage (off for tests and headless renders).
    explicit Game(std::vector<Level> levels, bool persist = true);

    void update(float dt, const Input& in) override;
    void render(Framebuffer& fb, float alpha) override;

    // Used by the headless showcase renderer and the tests.
    void startLevel(int index);
    void showScreen(Screen s);
    Screen screen() const { return screen_; }
    const World& world() const { return world_; }
    const Progress& progress() const { return progress_; }
    int focus() const { return focus_; }
    int attempt() const { return attempt_; }
    int levelCount() const { return (int)levels_.size(); }
    void setHitboxes(bool on) { progress_.hitboxes = on; }

private:
    // ---- update per screen (game.cpp) ----
    void updateMenu(const Input& in);
    void updateLevelSelect(const Input& in);
    void updateSettings(const Input& in);
    void updatePlaying(float dt, const Input& in);
    void updatePaused(const Input& in);
    void updateDying(float dt);
    void updateDead(const Input& in);
    void updateComplete(const Input& in);
    // Shared button-list navigation. Returns the index chosen this step, or -1.
    int navigate(const std::vector<Button>& buttons, const Input& in, bool vertical, bool clickAnywhereChoosesFocus);
    void retry();
    void save();
    void layoutScreen();

    // ---- render per screen (game_screens.cpp) ----
    void drawMenu(Framebuffer& fb);
    void drawLevelSelect(Framebuffer& fb);
    void drawSettings(Framebuffer& fb);
    void drawPlay(Framebuffer& fb, float alpha);
    void drawPausedOverlay(Framebuffer& fb);
    void drawDeadOverlay(Framebuffer& fb);
    void drawCompleteOverlay(Framebuffer& fb);
    void drawButtons(Framebuffer& fb);

    std::vector<Level> levels_;
    bool persist_;
    Progress progress_;
    Screen screen_ = Screen::Menu;
    std::vector<Button> buttons_;  // buttons of the current screen
    int focus_ = 0;
    float screenTime_ = 0.0f;      // seconds on the current screen
    float menuTime_ = 0.0f;        // drives menu / backdrop animation

    int levelIndex_ = 0;
    World world_;
    Camera cam_, prevCam_;
    Palette pal_;
    Effects effects_;
    Animation death_;
    Vec2 deathAt_;
    int attempt_ = 1;
    bool requireRelease_ = false;  // ignore jump until the key/button is released once
    bool newBest_ = false;
    bool confirmReset_ = false;
    float hintTime_ = -1.0f;       // >= 0 while the first-run jump hint is showing

    static constexpr int kPauseX = 904, kPauseY = 54, kPauseS = 36;  // on-screen pause button (touch)
};
