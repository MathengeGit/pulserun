// Game state machine: what each screen does with input. Drawing is in game_screens.cpp.
#include "game/game.h"
#include "core/storage.h"
#include "game/finale.h"
#include <algorithm>
#include <cmath>

Game::Game(std::vector<Level> levels, bool persist) : levels_(std::move(levels)), persist_(persist) {
    if (persist_) progress_ = Progress::parse(loadSaveText());
    pal_ = defaultPalette();
    world_.setRules(rules());
    if (!levels_.empty()) {
        world_.setLevel(&levels_[0]);
        cam_.reset(world_.player.pos, levels_[0].groundY);
        prevCam_ = cam_;
    }
    showScreen(Screen::Menu);
}

void Game::save() {
    if (persist_) writeSaveText(progress_.serialize());
}

void Game::setDifficulty(Difficulty d) {
    progress_.difficulty = (int)d;
    world_.setRules(rules());
    save();
}

namespace {
std::string modeLabel(Difficulty d) { return std::string("MODE: ") + difficultyName(d); }
Difficulty nextMode(Difficulty d) { return (Difficulty)(((int)d + 1) % kDifficultyCount); }
}  // namespace

// ---------- Screens and layout ----------

void Game::layoutScreen() {
    buttons_.clear();
    const Palette p = defaultPalette();
    switch (screen_) {
    case Screen::Menu: {
        // Hit areas for the three menu icons (drawn with the original menu widgets).
        Button levels{283, 256, 78, 78, "LEVELS", p.neon};
        Button play{414, 229, 132, 132, "PLAY", p.cube};
        Button settings{599, 256, 78, 78, "SETTINGS", p.neon2};
        buttons_ = {levels, play, settings};
        break;
    }
    case Screen::LevelSelect: {
        const int cw = 200, ch = 300, gap = 30, x0 = (W - (4 * cw + 3 * gap)) / 2;
        for (int i = 0; i < 4; ++i) {
            Button b{x0 + i * (cw + gap), 100, cw, ch, "", p.neon};
            b.enabled = i < (int)levels_.size();
            buttons_.push_back(b);
        }
        buttons_.push_back(Button{24, 476, 120, 40, "BACK", p.neon});
        buttons_.push_back(Button{W - 24 - 250, 476, 250, 40, modeLabel(difficulty()), p.accent});
        break;
    }
    case Screen::Settings:
        buttons_ = stackButtons({std::string("DIFFICULTY: ") + difficultyName(difficulty()),
                                 progress_.hitboxes ? "HITBOXES: ON" : "HITBOXES: OFF",
                                 confirmReset_ ? "PRESS AGAIN TO RESET" : "RESET PROGRESS", "BACK"},
                                W / 2, 146, 420, 48, 12, p.neon);
        break;
    case Screen::Paused:
        buttons_ = stackButtons({"RESUME", "RESTART", "MAIN MENU"}, W / 2, 210, 300, 54, 16, p.neon);
        break;
    case Screen::Dead:
        buttons_ = stackButtons({"RETRY", "MAIN MENU"}, W / 2, 300, 280, 54, 16, p.accent);
        break;
    case Screen::Complete: {
        // After the last level the first button rolls the finale instead.
        const bool finale = lastLevel() && progress_.allCompleted((int)levels_.size());
        buttons_ = stackButtons({finale ? "THE END" : "NEXT LEVEL", "REPLAY", "MAIN MENU"}, W / 2, 330, 300, 46, 10, p.cube);
        buttons_[0].enabled = finale || !lastLevel();
        break;
    }
    case Screen::Finale: {
        int x, y, w, h;
        finaleButtonArea(x, y, w, h);
        buttons_ = stackButtons({"PLAY AGAIN", "MAIN MENU"}, x + w / 2, y + 10, w - 20, 56, 18, p.neon);
        break;
    }
    default: break;
    }
}

void Game::showScreen(Screen s) {
    screen_ = s;
    screenTime_ = 0;
    if (s != Screen::Settings) confirmReset_ = false;
    layoutScreen();
    switch (s) {
    case Screen::Menu: focus_ = 1; break;  // PLAY
    case Screen::LevelSelect: focus_ = std::min(levelIndex_, (int)buttons_.size() - 1); break;
    case Screen::Complete: focus_ = buttons_[0].enabled ? 0 : 1; break;
    case Screen::Finale: finaleTime_ = 0; focus_ = 0; break;
    default: focus_ = 0; break;
    }
}

void Game::startLevel(int index) {
    if (levels_.empty()) return;
    levelIndex_ = std::clamp(index, 0, (int)levels_.size() - 1);
    world_.setLevel(&levels_[levelIndex_]);
    world_.setRules(rules());
    pal_ = levelPalette(levels_[levelIndex_]);
    attempt_ = 0;
    hintTime_ = (!progress_.seenHint && levelIndex_ == 0) ? 0.0f : -1.0f;
    retry();
}

void Game::retry() {
    world_.reset();
    cpTaken_ = 0;
    cpToast_ = -1;
    cam_.reset(world_.player.pos, world_.level->groundY);
    prevCam_ = cam_;
    effects_.clear();
    newBest_ = false;
    ++attempt_;
    requireRelease_ = true;
    showScreen(Screen::Playing);
}

// EASY: respawn standing on the ground at the last checkpoint flag passed.
void Game::respawnAtCheckpoint() {
    const float clock = world_.clock;  // the run time keeps counting across respawns
    world_.spawnAt(world_.level->checkpoints[cpTaken_ - 1]);
    world_.clock = clock;
    cam_.reset(world_.player.pos, world_.level->groundY);
    cam_.follow(world_.player.pos, world_.level->groundY, 1.0f);
    prevCam_ = cam_;
    effects_.clear();
    newBest_ = false;
    ++attempt_;
    requireRelease_ = true;
    cpToast_ = 0;
    showScreen(Screen::Playing);
}

// ---------- Input helpers ----------

int Game::navigate(const std::vector<Button>& b, const Input& in, bool vertical, bool clickAnywhere) {
    if (b.empty()) return -1;
    if (vertical ? in.upPressed : in.leftPressed) focus_ = moveFocus(b, focus_, -1);
    if (vertical ? in.downPressed : in.rightPressed) focus_ = moveFocus(b, focus_, +1);
    if (in.clicked) {
        const int hit = buttonAt(b, in.pointerX, in.pointerY);
        if (hit >= 0) {
            focus_ = hit;
            return hit;
        }
        return clickAnywhere && b[focus_].enabled ? focus_ : -1;
    }
    // Space is a jump key in play, and "press" on menus (Enter too).
    if ((in.enterPressed || in.jumpPressed) && focus_ >= 0 && b[focus_].enabled) return focus_;
    return -1;
}

// ---------- Update ----------

void Game::update(float dt, const Input& in) {
    screenTime_ += dt;
    menuTime_ += dt;
    prevCam_ = cam_;
    effects_.update(dt);
    if (requireRelease_ && !in.jumpHeld) requireRelease_ = false;
    switch (screen_) {
    case Screen::Menu: updateMenu(in); break;
    case Screen::LevelSelect: updateLevelSelect(in); break;
    case Screen::Settings: updateSettings(in); break;
    case Screen::Playing: updatePlaying(dt, in); break;
    case Screen::Paused: updatePaused(in); break;
    case Screen::Dying: updateDying(dt); break;
    case Screen::Dead: updateDead(in); break;
    case Screen::Complete: updateComplete(in); break;
    case Screen::Finale:
        finaleTime_ += dt;
        updateFinale(in);
        break;
    }
}

void Game::updateMenu(const Input& in) {
    switch (navigate(buttons_, in, false, true)) {
    case 0: showScreen(Screen::LevelSelect); break;
    case 1: startLevel(levelIndex_); break;
    case 2: showScreen(Screen::Settings); break;
    default: break;
    }
}

void Game::updateLevelSelect(const Input& in) {
    if (in.escPressed) return showScreen(Screen::Menu);
    // Up/down hops between the cards and BACK; left/right moves along the cards.
    if (in.downPressed) focus_ = 4;
    if (in.upPressed && focus_ == 4) focus_ = std::min(levelIndex_, (int)levels_.size() - 1);
    Input lr = in;
    lr.upPressed = lr.downPressed = false;
    const int pick = navigate(buttons_, lr, false, false);
    if (pick == 4) {
        showScreen(Screen::Menu);
    } else if (pick == 5) {
        setDifficulty(nextMode(difficulty()));
        layoutScreen();
        focus_ = 5;
    } else if (pick >= 0) {
        startLevel(pick);
    }
}

void Game::updateSettings(const Input& in) {
    if (in.escPressed) return showScreen(Screen::Menu);
    const int pick = navigate(buttons_, in, true, false);
    if (pick == 0) {
        setDifficulty(nextMode(difficulty()));
    } else if (pick == 1) {
        progress_.hitboxes = !progress_.hitboxes;
        save();
    } else if (pick == 2) {
        if (confirmReset_) {
            const bool hb = progress_.hitboxes;
            progress_ = Progress{};
            progress_.hitboxes = hb;
            confirmReset_ = false;
            save();
        } else {
            confirmReset_ = true;
        }
    } else if (pick == 3) {
        return showScreen(Screen::Menu);
    }
    if (pick >= 0) {
        const int keep = focus_;
        layoutScreen();
        focus_ = keep;
    }
}

void Game::updatePlaying(float dt, const Input& in) {
    const bool pauseTap = in.clicked && in.pointerX >= kPauseX && in.pointerX < kPauseX + kPauseS &&
                          in.pointerY >= kPauseY && in.pointerY < kPauseY + kPauseS;
    if (in.escPressed || in.focusLost || pauseTap) return showScreen(Screen::Paused);
    if (in.hitboxPressed) {
        progress_.hitboxes = !progress_.hitboxes;
        save();
    }
    if (in.restartPressed) {
        ++progress_.at(world_.level->number).attempts;
        save();
        return retry();
    }
    if (cpToast_ >= 0 && (cpToast_ += dt) > 1.4f) cpToast_ = -1;
    if (hintTime_ >= 0) {
        hintTime_ += dt;
        if (hintTime_ > 4.0f) {
            hintTime_ = -1;
            progress_.seenHint = true;
            save();
        }
    }
    const bool held = in.jumpHeld && !requireRelease_;
    const bool pressed = in.jumpPressed && !requireRelease_;
    const WorldEvents ev = world_.step(dt, held, pressed);
    if (ev.orb) {
        const Obstacle& o = world_.level->obstacles[ev.orbIndex];
        effects_.spawn(makeOrbPop(pal_), Vec2(o.x, o.y));
    }
    if (ev.died) {
        newBest_ = progress_.recordDeath(world_.level->number, world_.progress());
        save();
        death_ = makeDeathBurst(pal_, ClipRect{0, 0, W - 1.0, H - 1.0});
        deathAt_ = world_.player.pos;
        return showScreen(Screen::Dying);
    }
    if (ev.completed) {
        newBest_ = progress_.recordComplete(world_.level->number, world_.clock, progress_.difficulty);
        save();
        for (int i = 0; i < 3; ++i) {
            Animation ring = makeOrbPop(pal_);
            ring.elapsed = -0.12f * i;  // staggered
            effects_.spawn(ring, world_.player.pos);
        }
        return showScreen(Screen::Complete);
    }
    // EASY: passing a checkpoint flag saves it.
    const std::vector<float>& cps = world_.level->checkpoints;
    if (world_.rules.checkpoints && cpTaken_ < (int)cps.size() && world_.player.pos.x >= cps[cpTaken_]) {
        while (cpTaken_ < (int)cps.size() && world_.player.pos.x >= cps[cpTaken_]) ++cpTaken_;
        cpToast_ = 0;
        Animation ring = makeOrbPop(pal_);
        effects_.spawn(ring, world_.player.pos);
    }
    cam_.follow(world_.player.pos, world_.level->groundY, dt);
}

void Game::updatePaused(const Input& in) {
    if (in.escPressed) {
        requireRelease_ = true;
        return showScreen(Screen::Playing);
    }
    switch (navigate(buttons_, in, true, false)) {
    case 0:
        requireRelease_ = true;
        showScreen(Screen::Playing);
        break;
    case 1: retry(); break;
    case 2: showScreen(Screen::Menu); break;
    default: break;
    }
}

void Game::updateDying(float dt) {
    death_.update(dt);
    const float shake = std::max(0.0f, 0.3f - screenTime_) * 36.0f;
    cam_.shake_x = shake * std::sin(screenTime_ * 90.0f);
    cam_.shake_y = shake * std::cos(screenTime_ * 70.0f);
    // EASY skips the crashed screen: straight back to the last checkpoint (or the start).
    const bool quick = world_.rules.checkpoints;
    if (death_.finished() && screenTime_ > death_.duration() + (quick ? 0.1f : 0.25f)) {
        cam_.shake_x = cam_.shake_y = 0;
        if (!quick) showScreen(Screen::Dead);
        else if (cpTaken_ > 0) respawnAtCheckpoint();
        else retry();
    }
}

void Game::updateDead(const Input& in) {
    if (in.escPressed) return showScreen(Screen::Menu);
    if (in.restartPressed) return retry();
    if (screenTime_ < 0.15f) return;  // swallow presses from the moment of death
    // Retry by itself after a moment, unless the player has moved to MAIN MENU.
    if (focus_ == 0 && screenTime_ >= kAutoRetry) return retry();
    switch (navigate(buttons_, in, true, true)) {
    case 0: retry(); break;
    case 1: showScreen(Screen::Menu); break;
    default: break;
    }
}

void Game::updateComplete(const Input& in) {
    if (in.escPressed) return showScreen(Screen::Menu);
    if (screenTime_ < 0.4f) return;
    const bool finale = lastLevel() && buttons_[0].enabled;
    if (finale && focus_ == 0 && screenTime_ >= kFinaleDelay) return showScreen(Screen::Finale);
    switch (navigate(buttons_, in, true, false)) {
    case 0:
        if (finale) showScreen(Screen::Finale);
        else startLevel(levelIndex_ + 1);
        break;
    case 1: startLevel(levelIndex_); break;
    case 2: showScreen(Screen::Menu); break;
    default: break;
    }
}

void Game::updateFinale(const Input& in) {
    if (in.escPressed) return showScreen(Screen::Menu);
    if (finaleTime_ < finaleIntroDuration()) {
        // Any press skips to the end of the intro (the picture is a pure function of time).
        if ((in.jumpPressed || in.enterPressed || in.clicked) && finaleTime_ > 0.5f) {
            finaleTime_ = finaleIntroDuration();
            requireRelease_ = true;
        }
        return;
    }
    if (requireRelease_) return;
    switch (navigate(buttons_, in, true, false)) {
    case 0: startLevel(0); break;
    case 1: showScreen(Screen::Menu); break;
    default: break;
    }
}
