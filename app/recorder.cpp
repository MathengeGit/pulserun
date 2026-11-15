// Demo-video recorder (pulserun --record <dir> [--ffmpeg <path>]). See recorder.h.
#include "recorder.h"
#include "core/storage.h"
#include "game/game.h"
#include "game/game_loop.h"
#include "game/solver.h"
#include "raster/font.h"
#include "ui/button.h"
#include "ui/ui.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

constexpr int W = Game::W, H = Game::H;
constexpr float kDt = (float)GameLoop::FIXED_DT;
const Color kInk = rgb(240, 244, 255), kDim = rgb(170, 178, 225), kShadow = rgb(6, 8, 30);

Color modeColour(Difficulty d) {
    return d == Difficulty::Easy ? rgb(120, 232, 150) : d == Difficulty::Normal ? rgb(255, 214, 90) : rgb(255, 96, 120);
}

// ---------- ffmpeg pipe ----------

class Encoder {
public:
    bool open(const std::string& ffmpeg, const std::string& path) {
        std::string cmd = "\"" + ffmpeg + "\" -y -loglevel error -f rawvideo -pix_fmt rgb24 -s " + std::to_string(W) +
                          "x" + std::to_string(H) + " -r 60 -i - -c:v libx264 -preset medium -crf 20 " +
                          "-pix_fmt yuv420p -movflags +faststart \"" + path + "\"";
#ifdef _WIN32
        cmd = "\"" + cmd + "\"";  // cmd.exe strips one outer pair of quotes
        pipe_ = _popen(cmd.c_str(), "wb");
#else
        pipe_ = popen(cmd.c_str(), "w");
#endif
        path_ = path;
        rgb_.resize((size_t)W * H * 3);
        frames_ = 0;
        return pipe_ != nullptr;
    }
    // `fade` 0..1 darkens the frame (fades in and out of clips).
    void frame(const Framebuffer& fb, float fade = 1.0f) {
        if (!pipe_) return;
        const Color* px = fb.data();
        const int k = (int)std::lround(std::clamp(fade, 0.0f, 1.0f) * 256);
        for (size_t i = 0; i < (size_t)W * H; ++i) {
            rgb_[i * 3 + 0] = (uint8_t)((px[i].r * k) >> 8);
            rgb_[i * 3 + 1] = (uint8_t)((px[i].g * k) >> 8);
            rgb_[i * 3 + 2] = (uint8_t)((px[i].b * k) >> 8);
        }
        std::fwrite(rgb_.data(), 1, rgb_.size(), pipe_);
        ++frames_;
    }
    bool close() {
        if (!pipe_) return false;
#ifdef _WIN32
        const int rc = _pclose(pipe_);
#else
        const int rc = pclose(pipe_);
#endif
        pipe_ = nullptr;
        std::printf("Wrote %s (%d frames, %.1f s)\n", path_.c_str(), frames_, frames_ / 60.0);
        return rc == 0;
    }
    int frames() const { return frames_; }

private:
    FILE* pipe_ = nullptr;
    std::string path_;
    std::vector<uint8_t> rgb_;
    int frames_ = 0;
};

// Every frame goes to the clip and to the demo reel.
struct Studio {
    std::string ffmpeg, dir;
    Encoder clip, reel;
    Framebuffer fb{W, H};
    int failures = 0;
    int clipStart = 0;   // reel frame index where the current clip began
    int fadeFrames = 18; // 0.3 s fades at both ends of each clip

    bool begin(const std::string& name) {
        if (!clip.open(ffmpeg, dir + "/" + name)) {
            std::fprintf(stderr, "Cannot start ffmpeg for %s\n", name.c_str());
            ++failures;
            return false;
        }
        return true;
    }
    void end() {
        if (!clip.close()) ++failures;
    }
    void emit(float fade = 1.0f) {
        const int n = clip.frames();
        const float in = std::min(1.0f, (n + 1) / (float)fadeFrames);
        clip.frame(fb, std::min(fade, in));
        reel.frame(fb, std::min(fade, in));
    }
    // Fade the last frame out over the fade length.
    void fadeOut() {
        for (int i = fadeFrames - 1; i >= 0; --i) emit((float)i / fadeFrames);
    }
};

// ---------- Title cards (drawn with the game's own backdrop and bitmap font) ----------

void titleCard(Studio& s, float seconds, const std::string& kicker, const std::string& title, Color titleColour,
               const std::vector<std::string>& lines) {
    const Palette pal = defaultPalette();
    const int n = (int)(seconds * 60);
    for (int f = 0; f < n; ++f) {
        const float t = f / 60.0f;
        drawBackdrop(s.fb, 600 + t * 40, 470, pal);
        drawGround(s.fb, 470, 600 + t * 40, pal);
        drawPulseLine(s.fb, 120, 840, 120, 10.0f + 18.0f * std::exp(-std::fmod(t, 1.0f) * 6.0f), pal.neon);
        drawTextCentered(s.fb, W / 2, 160, kicker, 2, kDim);
        const int rise = (int)(16 * std::exp(-t * 7.0f));
        drawTextCentered(s.fb, W / 2 + 4, 196 + rise + 4, title, 7, kShadow);
        drawTextCentered(s.fb, W / 2, 196 + rise, title, 7, titleColour);
        for (size_t i = 0; i < lines.size(); ++i) {
            const float a = std::clamp((t - 0.25f - 0.12f * i) * 4.0f, 0.0f, 1.0f);
            drawTextCentered(s.fb, W / 2, 280 + (int)i * 30, lines[i], 2, withAlpha(kInk, (int)(255 * a)));
        }
        s.emit();
    }
}

// ---------- Driving the game ----------

Input planInput(const std::vector<uint8_t>& plan, size_t i, bool& prev) {
    Input in;
    in.jumpHeld = i < plan.size() && plan[i] != 0;
    in.jumpPressed = in.jumpHeld && !prev;
    prev = in.jumpHeld;
    return in;
}

// Advance one step and record it.
void stepShot(Studio& s, Game& g, const Input& in = Input{}) {
    g.update(kDt, in);
    g.render(s.fb, 1.0f);
    s.emit();
}

void hold(Studio& s, Game& g, float seconds, const Input& in = Input{}) {
    for (int i = 0; i < (int)(seconds * 60); ++i) stepShot(s, g, in);
}

// Play `plan` on the current level from step `from`; record from step `recordFrom` on.
// Stops when the plan runs out, the screen changes, or after `maxSteps` recorded steps.
size_t playPlan(Studio* s, Game& g, const std::vector<uint8_t>& plan, size_t from, size_t recordFrom,
                size_t maxSteps = (size_t)-1) {
    bool prev = from > 0 && plan[from - 1];
    size_t i = from, recorded = 0;
    for (; i < plan.size() && g.screen() == Screen::Playing && recorded < maxSteps; ++i) {
        const Input in = planInput(plan, i, prev);
        if (s && i >= recordFrom) {
            stepShot(*s, g, in);
            ++recorded;
        } else {
            g.update(kDt, in);
        }
    }
    return i;
}

// Keep running (no input) until the level is finished, recording.
void runOut(Studio& s, Game& g) {
    for (int i = 0; i < 600 && g.screen() == Screen::Playing; ++i) stepShot(s, g);
}

// Play on from the game's current world state (e.g. after a respawn) with a fresh plan.
void continueFromHere(Studio& s, Game& g, float seconds) {
    // Let the jump key count as released first, as a human would after a respawn.
    stepShot(s, g);
    const SolveResult r = solveFrom(g.world(), 1e30f, 400000);
    if (!r.solved) std::fprintf(stderr, "Recorder: no plan from x=%.0f\n", g.world().player.pos.x);
    playPlan(&s, g, r.held, 0, 0, (size_t)(seconds * 60));
}

// ---------- Clips ----------

void clipLevelComplete(Studio& s, const std::vector<Level>& levels) {
    if (!s.begin("01_level_complete.mp4")) return;
    titleCard(s, 2.4f, "PULSERUN  -  DEMO 1", "LEVEL COMPLETE", rgb(120, 232, 150),
              {"WHAT HAPPENS WHEN YOU REACH THE PORTAL", "LEVEL 1  PRIMER  -  NORMAL MODE"});
    Game g(levels, false);
    g.startLevel(0);
    const SolveResult plan = solveLevel(levels[0], 400000, g.rules());
    const size_t tail = (size_t)(6.5f * 60);
    playPlan(&s, g, plan.held, 0, plan.held.size() > tail ? plan.held.size() - tail : 0);
    runOut(s, g);
    hold(s, g, 4.5f);  // rings, stats, NEXT LEVEL focused
    Input enter;
    enter.enterPressed = true;
    stepShot(s, g, enter);
    const SolveResult plan2 = solveLevel(levels[1], 400000, g.rules());
    playPlan(&s, g, plan2.held, 0, 0, (size_t)(3.0f * 60));
    s.fadeOut();
    s.end();
}

const char* modeLine(Difficulty d) {
    switch (d) {
    case Difficulty::Easy: return "WORLD AT 76% SPEED  -  WIDE HITBOXES  -  CHECKPOINTS";
    case Difficulty::Normal: return "WORLD AT 88% SPEED  -  FAIRER HITBOXES  -  AUTO RETRY";
    default: return "THE ORIGINAL TUNING  -  FULL SPEED  -  NO MERCY";
    }
}
const char* modeLine2(Difficulty d) {
    switch (d) {
    case Difficulty::Easy: return "CRASH = BACK TO THE LAST FLAG, NOT THE START";
    case Difficulty::Normal: return "COYOTE TIME + LONGER JUMP BUFFER";
    default: return "WATCH THE HITBOXES: FULL-SIZE SPIKES AND SAWS";
    }
}

void clipDifficulty(Studio& s, const std::vector<Level>& levels, Difficulty d, const char* file, int demoNo) {
    if (!s.begin(file)) return;
    titleCard(s, 2.6f, "PULSERUN  -  DEMO " + std::to_string(demoNo), difficultyName(d), modeColour(d),
              {modeLine(d), modeLine2(d), "LEVEL 1  PRIMER"});
    Game g(levels, false);
    g.setDifficulty(d);
    g.startLevel(0);
    const SolveResult plan = solveLevel(levels[0], 400000, g.rules());
    // A clean stretch, with the collision overlay switched on for part of it.
    size_t at = playPlan(&s, g, plan.held, 0, 0, (size_t)(4.5f * 60));
    g.setHitboxes(true);
    at = playPlan(&s, g, plan.held, at, 0, (size_t)(3.5f * 60));
    g.setHitboxes(false);
    if (d == Difficulty::Easy) {
        // Past the first checkpoint, then a deliberate crash: straight back to the flag.
        while (g.checkpointsTaken() < 1 && g.screen() == Screen::Playing) at = playPlan(&s, g, plan.held, at, 0, 1);
        at = playPlan(&s, g, plan.held, at, 0, (size_t)(1.0f * 60));
        for (int i = 0; i < 900 && g.screen() == Screen::Playing; ++i) stepShot(s, g);  // stop pressing
        for (int i = 0; i < 200 && g.screen() != Screen::Playing; ++i) stepShot(s, g);
        continueFromHere(s, g, 5.0f);
    } else if (d == Difficulty::Normal) {
        // A crash: the crashed screen counts down and retries by itself.
        for (int i = 0; i < 900 && g.screen() == Screen::Playing; ++i) stepShot(s, g);
        for (int i = 0; i < 400 && g.screen() != Screen::Playing; ++i) stepShot(s, g);
        playPlan(&s, g, plan.held, 0, 0, (size_t)(3.0f * 60));
    } else {
        playPlan(&s, g, plan.held, at, 0, (size_t)(6.0f * 60));
    }
    s.fadeOut();
    s.end();
}

// Average 2x2 blocks: draw a full frame at half size into `dst` at (ox, oy).
void blitHalf(const Framebuffer& src, Framebuffer& dst, int ox, int oy) {
    const Color* p = src.data();
    for (int y = 0; y < H / 2; ++y)
        for (int x = 0; x < W / 2; ++x) {
            const Color a = p[(2 * y) * W + 2 * x], b = p[(2 * y) * W + 2 * x + 1];
            const Color c = p[(2 * y + 1) * W + 2 * x], e = p[(2 * y + 1) * W + 2 * x + 1];
            dst.setPixel(ox + x, oy + y,
                         rgb((a.r + b.r + c.r + e.r) / 4, (a.g + b.g + c.g + e.g) / 4, (a.b + b.b + c.b + e.b) / 4));
        }
}

void clipCompare(Studio& s, const std::vector<Level>& levels) {
    if (!s.begin("05_difficulty_compare.mp4")) return;
    titleCard(s, 2.6f, "PULSERUN  -  DEMO 5", "SIDE BY SIDE", rgb(0, 238, 255),
              {"EASY, NORMAL AND HARD RUNNING AT THE SAME REAL TIME", "SAME LEVEL, SAME JUMP ARCS - ONLY THE CLOCK AND",
               "THE HITBOXES CHANGE, SO EVERY LEVEL STAYS BEATABLE"});
    struct Lane {
        std::unique_ptr<Game> game;
        std::vector<uint8_t> plan;
        size_t at = 0;
        bool prev = false;
        Difficulty d;
    };
    std::vector<Lane> lanes;
    for (Difficulty d : {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard}) {
        Lane l;
        l.game.reset(new Game(levels, false));
        l.game->setDifficulty(d);
        l.game->startLevel(0);
        l.game->setHitboxes(true);
        l.plan = solveLevel(levels[0], 400000, l.game->rules()).held;
        l.d = d;
        lanes.push_back(std::move(l));
    }
    Framebuffer lane(W, H);
    const Palette pal = defaultPalette();
    const int cells[3][2] = {{0, 0}, {W / 2, 0}, {0, H / 2}};
    for (int f = 0; f < 15 * 60; ++f) {
        for (Lane& l : lanes) {
            l.game->update(kDt, planInput(l.plan, l.at++, l.prev));
        }
        for (int i = 0; i < 3; ++i) {
            lanes[i].game->render(lane, 1.0f);
            blitHalf(lane, s.fb, cells[i][0], cells[i][1]);
            const char* name = difficultyName(lanes[i].d);
            fillRoundedRect(s.fb, cells[i][0] + 10, cells[i][1] + 34, textWidth(name, 3) + 20, 34, 6, rgba(6, 8, 30, 200));
            drawText(s.fb, cells[i][0] + 20, cells[i][1] + 42, name, 3, modeColour(lanes[i].d));
        }
        // Fourth cell: how far each mode has got after the same real time.
        const int ox = W / 2, oy = H / 2;
        s.fb.fillRect(ox, oy, W / 2, H / 2, rgb(10, 12, 40));
        drawText(s.fb, ox + 24, oy + 22, "SAME REAL TIME:", 2, kDim);
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.1f S", f / 60.0f);
        drawText(s.fb, ox + 24, oy + 46, buf, 4, kInk);
        float far = 1;
        for (const Lane& l : lanes) far = std::max(far, l.game->world().player.pos.x);
        for (int i = 0; i < 3; ++i) {
            const World& w = lanes[i].game->world();
            const int y = oy + 104 + i * 46;
            drawText(s.fb, ox + 24, y, difficultyName(lanes[i].d), 2, modeColour(lanes[i].d));
            const int bw = 250;
            drawProgressBar(s.fb, ox + 120, y - 2, bw, 16, w.progress(), modeColour(lanes[i].d), rgb(8, 10, 34), kDim);
            std::snprintf(buf, sizeof buf, "%d%%", (int)std::lround(w.progress() * 100));
            drawText(s.fb, ox + 120 + bw + 12, y, buf, 2, kInk);
        }
        drawText(s.fb, ox + 24, oy + 248, "HITBOXES ON: SPIKES SHRINK ON EASY", 2, kDim);
        s.fb.fillRect(W / 2 - 1, 0, 2, H, pal.neon);
        s.fb.fillRect(0, H / 2 - 1, W, 2, pal.neon);
        s.emit();
    }
    s.fadeOut();
    s.end();
}

void clipFinale(Studio& s, const std::vector<Level>& levels) {
    if (!s.begin("06_finale.mp4")) return;
    titleCard(s, 2.6f, "PULSERUN  -  DEMO 6", "THE END", rgb(255, 214, 90),
              {"BEATING THE WHOLE GAME", "THE LAST SECONDS OF LEVEL 3, THEN THE FINALE"});
    Game g(levels, false);
    const int last = (int)levels.size() - 1;
    for (int i = 0; i < last; ++i) {  // clear the earlier levels off camera
        g.startLevel(i);
        playPlan(nullptr, g, solveLevel(levels[i], 400000, g.rules()).held, 0, (size_t)-1);
        for (int k = 0; k < 600 && g.screen() == Screen::Playing; ++k) g.update(kDt, Input{});
    }
    g.startLevel(last);
    const SolveResult plan = solveLevel(levels[last], 400000, g.rules());
    const size_t tail = (size_t)(6.0f * 60);
    playPlan(&s, g, plan.held, 0, plan.held.size() > tail ? plan.held.size() - tail : 0);
    runOut(s, g);
    // The complete screen counts down and rolls the finale by itself.
    for (int i = 0; i < 600 && g.screen() == Screen::Complete; ++i) stepShot(s, g);
    hold(s, g, 19.5f);  // intro (12 s), then the stats, credits and both credit pages
    s.fadeOut();
    s.end();
}

}  // namespace

int runRecord(const std::vector<Level>& levels, const std::string& dir, const std::string& ffmpeg) {
    setStorageEnabled(false);
    if (levels.size() < 2) return 1;
    Studio s;
    s.ffmpeg = ffmpeg;
    s.dir = dir;
    if (!s.reel.open(ffmpeg, dir + "/pulserun_demo_reel.mp4")) {
        std::fprintf(stderr, "Cannot run ffmpeg (%s). Pass --ffmpeg <path>.\n", ffmpeg.c_str());
        return 1;
    }
    clipLevelComplete(s, levels);
    clipDifficulty(s, levels, Difficulty::Easy, "02_easy.mp4", 2);
    clipDifficulty(s, levels, Difficulty::Normal, "03_normal.mp4", 3);
    clipDifficulty(s, levels, Difficulty::Hard, "04_hard.mp4", 4);
    clipCompare(s, levels);
    clipFinale(s, levels);
    if (!s.reel.close()) ++s.failures;
    return s.failures == 0 ? 0 : 1;
}
