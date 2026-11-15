// Drawing for every Game screen. Menu, level select and death follow the Checkpoint 1
// stills in docs/screenshots/ (07, 08, 09), now animated and interactive.
#include "game/game.h"
#include "game/finale.h"
#include "raster/circle.h"
#include "raster/font.h"
#include "raster/polygon.h"
#include "ui/ui.h"
#include <cmath>
#include <cstdio>

namespace {
const Color kText = rgb(240, 244, 255), kDim = rgb(170, 178, 225), kShadow = rgb(6, 8, 30);

void title(Framebuffer& fb, int cx, int y, const char* text, int scale, Color c) {
    drawTextCentered(fb, cx + 3, y + 3, text, scale, kShadow);
    drawTextCentered(fb, cx, y, text, scale, c);
}

std::string fmtTime(float s) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2fS", s);
    return buf;
}

// One colour per difficulty: green EASY, yellow NORMAL, red HARD (matches the finale).
Color modeColour(Difficulty d) {
    switch (d) {
    case Difficulty::Easy: return rgb(120, 232, 150);
    case Difficulty::Normal: return rgb(255, 214, 90);
    default: return rgb(255, 96, 120);
    }
}

const char* modeBlurb(Difficulty d) {
    switch (d) {
    case Difficulty::Easy: return "SLOWER  -  WIDER HITBOXES  -  CHECKPOINTS";
    case Difficulty::Normal: return "A LITTLE SLOWER  -  FAIRER HITBOXES";
    default: return "THE ORIGINAL TUNING  -  NO MERCY";
    }
}

// Focus ring for the round-cornered icon buttons on the menu.
void focusRing(Framebuffer& fb, const Button& b) {
    drawRoundedRect(fb, b.x - 9, b.y - 9, b.w + 18, b.h + 18, 20, rgb(245, 248, 255), 2);
}
}  // namespace

void Game::render(Framebuffer& fb, float alpha) {
    switch (screen_) {
    case Screen::Menu: drawMenu(fb); break;
    case Screen::LevelSelect: drawLevelSelect(fb); break;
    case Screen::Settings: drawSettings(fb); break;
    case Screen::Playing: drawPlay(fb, alpha); break;
    case Screen::Paused:
        drawPlay(fb, 0);
        drawPausedOverlay(fb);
        break;
    case Screen::Dying: drawPlay(fb, alpha); break;
    case Screen::Dead:
        drawPlay(fb, 0);
        drawDeadOverlay(fb);
        break;
    case Screen::Complete:
        drawPlay(fb, alpha);
        drawCompleteOverlay(fb);
        break;
    case Screen::Finale: drawFinaleScreen(fb); break;
    }
}

void Game::drawButtons(Framebuffer& fb) {
    for (size_t i = 0; i < buttons_.size(); ++i) drawButton(fb, buttons_[i], (int)i == focus_);
}

void Game::drawMenu(Framebuffer& fb) {
    const Palette pal = defaultPalette();
    const int groundY = 462;
    const float scroll = 900 + menuTime_ * 40.0f;
    drawBackdrop(fb, scroll, groundY, pal);
    drawGround(fb, groundY, scroll, pal);

    drawLogo(fb, W / 2, 62, 12, pal);
    // The ECG line beats: amplitude pulses once a second.
    const float beat = std::fmod(menuTime_, 1.0f);
    drawPulseLine(fb, 200, 760, 200, 18.0f + 26.0f * std::exp(-beat * 6.0f), pal.neon);

    drawPlayButton(fb, W / 2, 295, 132, pal);
    drawIconButton(fb, 322, 295, 78, pal.neon);
    for (int i = 0; i < 4; ++i)
        fillPolygon(fb, makeRect(322.0f - 22 + (i % 2) * 24, 295.0f - 22 + (i / 2) * 24, 20, 20), pal.neon);
    drawTextCentered(fb, 322, 350, "LEVELS", 2, rgb(200, 210, 255));
    drawIconButton(fb, 638, 295, 78, pal.neon2);
    drawSaw(fb, Vec2(638, 295), 26, menuTime_ * 1.5f, pal);
    drawTextCentered(fb, 638, 350, "SETTINGS", 2, rgb(200, 210, 255));
    if (focus_ >= 0 && focus_ < (int)buttons_.size()) focusRing(fb, buttons_[focus_]);

    // Blink the prompt (on 0.7 s, off 0.3 s).
    if (std::fmod(menuTime_, 1.0f) < 0.7f) drawTextCentered(fb, W / 2, 392, "PRESS SPACE OR TAP TO START", 3, kText);

    // The cube hops on the spot every 1.6 s with a half turn.
    const float t = std::fmod(menuTime_, 1.6f);
    const float hop = t < 0.6f ? 4.0f * 70.0f * (t / 0.6f) * (1 - t / 0.6f) : 0.0f;
    const float spin = t < 0.6f ? (t / 0.6f) * kPi / 2 : 0.0f;
    drawCube(fb, Vec2(150, groundY - 30.0f - hop), 60, spin, pal);
    drawSpike(fb, 810, (float)groundY, 52, 52, pal);
    drawSpike(fb, 862, (float)groundY, 52, 52, pal);
    drawTextCentered(fb, W / 2, 500, "ARROWS + ENTER  -  H HITBOXES  -  PURE C++ RENDERER", 2, rgb(150, 160, 215));
}

void Game::drawLevelSelect(Framebuffer& fb) {
    const Palette pal = defaultPalette();
    const int groundY = 486;
    const float scroll = 3200 + menuTime_ * 30.0f;
    drawBackdrop(fb, scroll, groundY, pal);
    drawGround(fb, groundY, scroll, pal);
    title(fb, W / 2, 30, "SELECT LEVEL", 6, kText);

    const char* fallback[4] = {"PRIMER", "ESCALATION", "ADVANCED", "EXPERT"};
    const Color accents[4] = {pal.cube, pal.neon, pal.neon2, rgb(120, 122, 160)};
    for (int i = 0; i < 4; ++i) {
        const Button& b = buttons_[i];
        const bool locked = i >= (int)levels_.size();
        const std::string name = locked ? fallback[i] : levels_[i].name;
        const float best = locked ? 0.0f : progress_.levels.size() > (size_t)i ? progress_.levels[i].best : 0.0f;
        const int lift = (i == focus_ && !locked) ? 8 : 0;  // focused card rises
        drawLevelCard(fb, b.x, b.y - lift, b.w, b.h, name, i + 1, locked ? 2 : levels_[i].difficulty, best, locked,
                      accents[i], pal);
        if (i == focus_ && !locked) drawRoundedRect(fb, b.x - 9, b.y - lift - 9, b.w + 18, b.h + 18, 22, kText, 2);
    }
    drawButton(fb, buttons_[4], focus_ == 4);
    if (buttons_.size() > 5) {
        Button mode = buttons_[5];
        mode.accent = modeColour(difficulty());
        drawButton(fb, mode, focus_ == 5);
    }
    // Which mode each level has been cleared on.
    for (int i = 0; i < 4 && i < (int)levels_.size(); ++i) {
        const int c = progress_.levels.size() > (size_t)i ? progress_.levels[i].clearedOn : -1;
        if (c < 0) continue;
        const Button& b = buttons_[i];
        drawTextCentered(fb, b.x + b.w / 2, b.y + b.h + 12, std::string("CLEARED ") + difficultyName((Difficulty)c), 2,
                         modeColour((Difficulty)c));
    }
    drawTextCentered(fb, W / 2, 440, "ARROWS TO CHOOSE  -  SPACE TO PLAY  -  ESC BACK", 2, rgb(200, 210, 255));
}

void Game::drawSettings(Framebuffer& fb) {
    const Palette pal = defaultPalette();
    drawBackdrop(fb, 1500 + menuTime_ * 30.0f, 486, pal);
    drawGround(fb, 486, 1500 + menuTime_ * 30.0f, pal);
    drawOverlayPanel(fb, 220, 60, 520, 420, pal.neon);
    title(fb, W / 2, 92, "SETTINGS", 5, kText);
    drawButtons(fb);
    drawTextCentered(fb, W / 2, 390, modeBlurb(difficulty()), 2, modeColour(difficulty()));
    drawTextCentered(fb, W / 2, 420, "SPACE / CLICK  JUMP (HOLD = HIGHER)", 2, kDim);
    drawTextCentered(fb, W / 2, 444, "ESC  PAUSE    R  RESTART    H  HITBOXES", 2, kDim);
}

void Game::drawPlay(Framebuffer& fb, float alpha) {
    if (!world_.level) return;
    const Camera cam = Camera::lerp(prevCam_, cam_, alpha);
    RenderOptions opt;
    opt.hitboxes = progress_.hitboxes;
    opt.drawPlayer = !world_.complete;
    opt.checkpoints = world_.rules.checkpoints;
    opt.checkpointsTaken = cpTaken_;
    drawWorld(fb, world_, cam, alpha, pal_, opt);
    effects_.draw(fb, cam.world_to_screen());
    if (screen_ == Screen::Dying || screen_ == Screen::Dead) {
        death_.draw(fb, cam.toScreen(deathAt_));
        const int flash = screen_ == Screen::Dying ? std::max(26, 70 - (int)(screenTime_ * 200)) : 26;
        fb.fillRect(0, 0, W, H, withAlpha(pal_.hazard, flash));
    }
    std::string mode = difficultyName(difficulty());
    if (world_.rules.checkpoints && !world_.level->checkpoints.empty())
        mode += "  CP " + std::to_string(cpTaken_) + "/" + std::to_string(world_.level->checkpoints.size());
    drawHud(fb, world_.level->number, world_.progress(), attempt_, pal_, mode, modeColour(difficulty()));
    if (cpToast_ >= 0 && screen_ == Screen::Playing) {
        const float a = std::min(1.0f, std::min(cpToast_ * 6.0f, (1.4f - cpToast_) * 3.0f));
        const int rise = (int)(10 * (1 - std::min(1.0f, cpToast_ * 4)));
        drawTextCentered(fb, W / 2 + 2, 92 + rise + 2, "CHECKPOINT", 3, withAlpha(kShadow, (int)(200 * a)));
        drawTextCentered(fb, W / 2, 92 + rise, "CHECKPOINT", 3, withAlpha(pal_.cube, (int)(255 * std::max(0.0f, a))));
    }
    if (screen_ == Screen::Playing) {
        // Pause button for touch screens (two bars).
        fillRoundedRect(fb, kPauseX, kPauseY, kPauseS, kPauseS, 8, rgba(8, 10, 34, 170));
        drawRoundedRect(fb, kPauseX, kPauseY, kPauseS, kPauseS, 8, rgb(235, 240, 255), 2);
        fb.fillRect(kPauseX + 11, kPauseY + 9, 5, 18, rgb(235, 240, 255));
        fb.fillRect(kPauseX + 20, kPauseY + 9, 5, 18, rgb(235, 240, 255));
    }
    if (hintTime_ >= 0 && screen_ == Screen::Playing) {
        const float a = std::min(1.0f, std::min(hintTime_ * 3.0f, (4.0f - hintTime_) * 2.0f));
        const Color c = withAlpha(kText, (int)(255 * std::max(0.0f, a)));
        drawTextCentered(fb, W / 2, 120, "SPACE / CLICK / TAP TO JUMP", 3, c);
        drawTextCentered(fb, W / 2, 152, "HOLD FOR A HIGHER JUMP  -  ESC TO PAUSE", 2, c);
    }
}

void Game::drawPausedOverlay(Framebuffer& fb) {
    drawOverlayPanel(fb, 290, 90, 380, 380, pal_.neon);
    title(fb, W / 2, 120, "PAUSED", 5, kText);
    char buf[48];
    std::snprintf(buf, sizeof buf, "%s  -  %d%%", world_.level->name.c_str(), (int)std::lround(world_.progress() * 100));
    drawTextCentered(fb, W / 2, 172, buf, 2, kDim);
    drawButtons(fb);
}

void Game::drawDeadOverlay(Framebuffer& fb) {
    char buf[48];
    std::snprintf(buf, sizeof buf, "CRASHED AT %d%%", (int)std::lround(world_.progress() * 100));
    title(fb, W / 2, 140, buf, 5, rgb(255, 235, 240));
    const LevelRecord& r = progress_.levels[world_.level->number - 1];
    std::snprintf(buf, sizeof buf, newBest_ ? "NEW BEST  %d%%" : "BEST  %d%%", (int)std::lround(r.best * 100));
    drawTextCentered(fb, W / 2, 200, buf, 3, newBest_ ? pal_.cube : kDim);
    drawTextCentered(fb, W / 2, 246, "TAP, SPACE OR R TO RETRY", 2, pal_.accent);
    drawButtons(fb);
    // Auto-retry countdown: a bar that drains under the RETRY button.
    if (focus_ == 0 && !buttons_.empty()) {
        const Button& b = buttons_[0];
        const float left = std::max(0.0f, 1.0f - screenTime_ / kAutoRetry);
        fb.fillRect(b.x + 10, b.y + b.h + 4, (int)((b.w - 20) * left), 4, pal_.accent);
    }
}

void Game::drawCompleteOverlay(Framebuffer& fb) {
    drawOverlayPanel(fb, 200, 50, 560, 460, pal_.cube);
    title(fb, W / 2, 80, "LEVEL", 6, kText);
    title(fb, W / 2, 132, "COMPLETE", 6, pal_.cube);
    const LevelRecord& r = progress_.levels[world_.level->number - 1];
    char buf[64];
    std::snprintf(buf, sizeof buf, "ATTEMPTS  %d", attempt_);
    drawTextCentered(fb, W / 2, 200, buf, 3, kText);
    drawTextCentered(fb, W / 2, 234, "TIME  " + fmtTime(world_.time), 3, kText);
    drawTextCentered(fb, W / 2, 268, (newBest_ ? "NEW BEST  " : "BEST  ") + fmtTime(r.bestTime), 3,
                     newBest_ ? pal_.accent : kDim);
    drawTextCentered(fb, W / 2, 296, std::string("ON ") + difficultyName(difficulty()), 2, modeColour(difficulty()));
    drawButtons(fb);
    if (lastLevel() && buttons_[0].enabled && focus_ == 0) {
        const float left = std::max(0.0f, 1.0f - screenTime_ / kFinaleDelay);
        const Button& b = buttons_[0];
        fb.fillRect(b.x + 10, b.y + b.h + 3, (int)((b.w - 20) * left), 3, pal_.accent);
    }
}

void Game::drawFinaleScreen(Framebuffer& fb) {
    FinaleStats st;
    st.difficulty = difficultyName(difficulty());
    for (size_t i = 0; i < levels_.size(); ++i) {
        const LevelRecord r = i < progress_.levels.size() ? progress_.levels[i] : LevelRecord{};
        st.levelNames.push_back(levels_[i].name);
        st.levelTimes.push_back(r.bestTime);
        st.levelAttempts.push_back(r.attempts);
        st.totalAttempts += r.attempts;
        st.totalTime += r.bestTime;
    }
    drawFinale(fb, finaleTime_, st, defaultPalette());
    if (finaleTime_ >= finaleIntroDuration()) drawButtons(fb);
    else if (finaleTime_ > 1.0f) drawTextCentered(fb, W - 120, H - 22, "SPACE TO SKIP", 2, withAlpha(kDim, 160));
}
