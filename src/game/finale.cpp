#include "game/finale.h"

#include "math/mat3.h"
#include "raster/circle.h"
#include "raster/clip.h"
#include "raster/curve.h"
#include "raster/font.h"
#include "raster/line.h"
#include "raster/polygon.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

// Timeline (seconds):
//   0.0 -  2.3  ESCAPE      cube bursts out of the last portal in slow motion (Bezier arc)
//   2.3 -  5.6  POWER DOWN  the world sinks and dims; one stamped card per beaten level
//   5.6 -  7.3  TITLE       "YOU BEAT PULSERUN" assembles from flying pixel blocks
//   7.3 -  9.0  FLATLINE    the pulse line draws flat, the cube lands on it and restarts it
//   8.45 - 11   FIREWORKS   Catmull-Rom rockets, midpoint-circle rings, clipped shards
//   9.3 - 12.0  SETTLE      title and heartbeat move up; stats, credits and buttons arrive
//  12.0 -       IDLE        cube hops each heartbeat, credit pages turn, quiet fireworks

namespace {

const int W = 960, H = 540;
const float kIntro = 12.0f;

// Button rectangle handed to the screen code.
const int kBtnX = 640, kBtnY = 206, kBtnW = 260, kBtnH = 150;

const Color kInk = {6, 8, 30, 255};
const Color kWhite = {235, 240, 255, 255};
const Color kDim = {150, 160, 215, 255};

// ---------------------------------------------------------------- timing helpers
float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
// Progress of t through [a, b], clamped to 0..1.
float seg(float t, float a, float b) { return clamp01((t - a) / (b - a)); }
float mix(float a, float b, float k) { return a + (b - a) * k; }
float easeOutCubic(float p) { const float q = 1 - p; return 1 - q * q * q; }
float easeOutQuart(float p) { const float q = 1 - p; return 1 - q * q * q * q; }
float easeInCubic(float p) { return p * p * p; }
float easeInOutCubic(float p) { return p < 0.5f ? 4 * p * p * p : 1 - std::pow(-2 * p + 2, 3.0f) / 2; }
float easeOutBack(float p) {
    const float c1 = 1.70158f, c3 = c1 + 1, q = p - 1;
    return 1 + c3 * q * q * q + c1 * q * q;
}

// Deterministic pseudo-random number in [0,1) from two integers (no rand(), no state).
float h01(int a, int b) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFF) / 16777216.0f;
}

int ir(float v) { return (int)std::lround(v); }
Color fade(Color c, float a) { return withAlpha(c, ir(c.a * clamp01(a))); }

std::string fmtTime(float s) {
    char buf[24];
    if (s < 0) s = 0;
    if (s < 99.95f) {
        std::snprintf(buf, sizeof buf, "%.1fS", s);
    } else {
        const int m = (int)(s / 60);
        std::snprintf(buf, sizeof buf, "%d:%04.1f", m, s - m * 60);
    }
    return buf;
}

// ---------------------------------------------------------------- bitmap text as geometry
// The font only draws axis-aligned text at integer scales. To move, scale and rotate
// lettering we rasterise the string once at scale 1 and read the lit pixels back, then
// push each one through a Mat3 and fill it with the scanline polygon filler.
struct Bits {
    int w = 0;
    std::vector<uint8_t> on;  // w * 7
    bool at(int x, int y) const { return on[(size_t)y * w + x] != 0; }
};

Bits rasterText(const std::string& s) {
    Bits b;
    b.w = std::max(1, textWidth(s, 1));
    Framebuffer tmp(b.w, 7);
    tmp.clear(rgb(0, 0, 0));
    drawText(tmp, 0, 0, s, 1, rgb(255, 255, 255));
    b.on.resize((size_t)b.w * 7);
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < b.w; ++x) b.on[(size_t)y * b.w + x] = tmp.getPixel(x, y).r > 127;
    return b;
}

// Text whose local pixel grid (1 unit per font pixel, origin top-left) is mapped by M.
void drawTextM(Framebuffer& fb, const Mat3& M, const std::string& s, Color c) {
    const Bits b = rasterText(s);
    for (int y = 0; y < 7; ++y) {
        for (int x = 0; x < b.w;) {
            if (!b.at(x, y)) { ++x; continue; }
            int x1 = x;
            while (x1 < b.w && b.at(x1, y)) ++x1;  // merge a horizontal run into one quad
            const Polygon q{M * Vec2((float)x, (float)y), M * Vec2((float)x1, (float)y),
                            M * Vec2((float)x1, (float)y + 1), M * Vec2((float)x, (float)y + 1)};
            fillPolygon(fb, q, c);
            x = x1;
        }
    }
}

void textRight(Framebuffer& fb, int xRight, int y, const std::string& s, int scale, Color c) {
    drawText(fb, xRight - textWidth(s, scale), y, s, scale, c);
}

// ---------------------------------------------------------------- the heartbeat
// One ECG period as (phase, height) pairs; the same shape drawPulseLine uses.
const float kEcg[][2] = {{0.00f, 0},      {0.30f, 0},     {0.34f, 0.18f}, {0.38f, 0},     {0.42f, 0},
                         {0.455f, -0.25f}, {0.50f, 1.0f},  {0.55f, -0.55f}, {0.585f, 0},   {0.63f, 0},
                         {0.68f, 0.28f},  {0.74f, 0},     {1.00f, 0}};
const int kEcgN = (int)(sizeof kEcg / sizeof kEcg[0]);

float ecg(float u) {
    u -= std::floor(u);
    for (int i = 0; i + 1 < kEcgN; ++i)
        if (u <= kEcg[i + 1][0]) {
            const float k = (u - kEcg[i][0]) / (kEcg[i + 1][0] - kEcg[i][0]);
            return mix(kEcg[i][1], kEcg[i + 1][1], k);
        }
    return 0;
}

const float kBeatPx = 200.0f;     // one heartbeat is this wide on screen...
const float kBeatSpeed = 200.0f;  // ...and scrolls this fast, so the cube hops once a second
const float kRideStart = 9.0f;    // the line starts scrolling
const float kLand = 7.9f;         // the cube touches the flat line
const float kThump2 = 8.45f;

// Sharp attack, exponential decay: one "thump" of the restarting heart.
float thumpEnv(float tau) {
    if (tau < 0) return 0;
    return tau < 0.07f ? tau / 0.07f : std::exp(-(tau - 0.07f) * 6.5f);
}
float thumpAmp(float t) {
    return (55.0f * thumpEnv(t - kLand) + 95.0f * thumpEnv(t - kThump2)) * (1 - seg(t, 8.8f, kRideStart));
}

// Shared "settle" progress: title and heartbeat travel from centre stage to the header.
float settle(float t) { return easeInOutCubic(seg(t, 9.3f, 10.1f)); }
float lineBaseY(float t) { return mix(350.0f, 132.0f, settle(t)); }
float rideAmp(float t) { return mix(60.0f, 22.0f, settle(t)) * easeOutCubic(seg(t, kRideStart, 9.6f)); }
// Unwrapped beat phase under screen column x (the cube sits at x = 480).
float ridePhase(float x, float t) { return (x - 480.0f + kBeatSpeed * (t - kRideStart)) / kBeatPx + 0.75f; }

void drawHeartbeat(Framebuffer& fb, float t, const Palette& pal) {
    if (t < 7.3f) return;
    if (t < kRideStart) {
        // Flatline draws on, then the two restart thumps drawn with the menu's own drawPulseLine.
        const float p = easeInOutCubic(seg(t, 7.3f, 7.85f));
        const Color c = lerpColor(rgb(120, 126, 160), pal.neon, seg(t, kLand, kLand + 0.12f));
        if (t < kLand) {
            drawGlowPolyline(fb, {Vec2(0, 350), Vec2(std::max(2.0f, W * p), 350)}, c, 4.0f, 4);
            fillCircle(fb, ir(W * p), 350, 5, withAlpha(kWhite, 220));  // the pen
        } else {
            drawPulseLine(fb, 0, W, 350, thumpAmp(t), c);
        }
        return;
    }
    const float base = lineBaseY(t), amp = rideAmp(t);
    const float ph0 = ridePhase(0, t), ph1 = ridePhase((float)W, t);
    std::vector<Vec2> pts;
    pts.push_back(Vec2(0, base - amp * ecg(ph0)));
    for (int k = (int)std::floor(ph0); k <= (int)std::floor(ph1); ++k)
        for (int j = 0; j + 1 < kEcgN; ++j) {
            const float ph = k + kEcg[j][0];
            if (ph <= ph0 || ph >= ph1) continue;
            pts.push_back(Vec2((ph - ph0) * kBeatPx, base - amp * kEcg[j][1]));
        }
    pts.push_back(Vec2((float)W, base - amp * ecg(ph1)));
    const float s = settle(t);
    drawGlowPolyline(fb, pts, pal.neon, mix(4.0f, 3.0f, s), 3);
}

// ---------------------------------------------------------------- the cube's whole journey
struct CubePose {
    Vec2 pos;
    float size = 44, angle = 0;
};

const Vec2 kPortal(230, 340);
const Vec2 kB0(230, 340), kB1(400, 345), kB2(400, 95), kB3(480, 120);  // escape arc (cubic Bezier)

float escapeU(float t) { return easeOutQuart(seg(t, 0.3f, 2.3f)); }  // fast burst, long slow-motion tail

CubePose cubePose(float t) {
    CubePose c;
    if (t < 2.3f) {
        const float u = escapeU(t);
        c.pos = bezierPoint(kB0, kB1, kB2, kB3, u);
        c.size = 44.0f * mix(0.45f, 1.0f, seg(t, 0.3f, 0.55f));
        c.angle = u * 4.0f * kPi;  // two full turns, landing upright
        return c;
    }
    const float bob = 5.0f * std::sin((t - 2.3f) * 2.2f) * (1 - seg(t, 7.3f, 7.7f));
    if (t < 7.7f) {
        c.pos = Vec2(480, mix(120.0f, 292.0f, easeInOutCubic(seg(t, 5.5f, 6.3f))) + bob);
        return c;
    }
    const float lineTop = 350.0f - 2.0f - 22.0f;  // cube centre when resting on the flat line
    if (t < kLand) {
        c.pos = Vec2(480, mix(292.0f, lineTop, easeInCubic(seg(t, 7.7f, kLand))));
        return c;
    }
    if (t < kRideStart) {
        c.pos = Vec2(480, lineTop - thumpAmp(t));  // kicked up by each thump
        return c;
    }
    // Riding: sit on the line, and hop over every spike exactly like in the game.
    const float amp = rideAmp(t), ph = ridePhase(480, t), u = ph - std::floor(ph);
    c.size = mix(44.0f, 24.0f, settle(t));
    const float q = clamp01((u - 0.36f) / 0.28f);
    const float jump = (u > 0.36f && u < 0.64f) ? 4.0f * q * (1 - q) * (amp + 10.0f) : 0.0f;
    const float h = std::max(amp * ecg(u), jump);
    c.pos = Vec2(480, lineBaseY(t) - 2.0f - c.size / 2 - h);
    const float pj = ph - 0.36f;  // one quarter turn per hop, snapping flat on landing
    c.angle = (kPi / 2) * (std::floor(pj) + easeInOutCubic(clamp01((pj - std::floor(pj)) / 0.28f)) - 1.0f);
    return c;
}

// ---------------------------------------------------------------- beat 1: escape
void drawEscape(Framebuffer& fb, float t, const Palette& pal) {
    if (t > 3.1f) return;
    const float u = escapeU(t);
    const ClipRect view{0, 0, (double)W - 1, (double)H - 1};

    // Speed lines: fast while the cube is fast, gone as slow motion takes over.
    const float streak = seg(t, 0.3f, 0.45f) * (1 - seg(t, 0.9f, 2.0f));
    if (streak > 0)
        for (int i = 0; i < 26; ++i) {
            const float y = 60 + h01(i, 1) * 340, len = 60 + h01(i, 2) * 160, sp = 900 + h01(i, 3) * 900;
            const float x = W + 200 - std::fmod(t * sp + h01(i, 4) * 2400, (float)W + 400);
            drawLineClipped(fb, x, y, x + len, y, view, withAlpha(kWhite, ir(70 * streak)));
        }

    // Portal: charges, spits the cube out, then collapses to nothing.
    const float collapse = 1 - easeInCubic(seg(t, 1.5f, 2.2f));
    if (collapse > 0.03f) {
        const float kick = 1 + 0.35f * thumpEnv(t - 0.3f);
        if (t < 0.3f)  // rings converge on the portal before the burst (midpoint circles)
            for (int r = 0; r < 3; ++r) {
                const float p = std::fmod(t / 0.3f + r / 3.0f, 1.0f);
                drawCircleThick(fb, ir(kPortal.x), ir(kPortal.y), ir(130 * (1 - p)) + 8, 2,
                                withAlpha(pal.neon2, ir(150 * p)));
            }
        drawPortal(fb, kPortal, 34 * kick * collapse, 88 * (0.4f + 0.6f * collapse) * collapse + 2, pal.neon2);
    }
    if (t >= 0.3f && t < 1.5f) {
        Palette burst = pal;
        burst.cube = pal.neon2;
        drawDeathBurst(fb, kPortal, easeOutCubic(seg(t, 0.3f, 1.5f)), burst, view);
    }

    // Trail along the Bezier arc; its tail catches up with the cube once it hovers.
    const float u0 = u * easeInOutCubic(seg(t, 2.2f, 3.0f));
    if (t >= 0.3f && u - u0 > 0.004f) {
        const int n = 40;
        Vec2 prev = bezierPoint(kB0, kB1, kB2, kB3, u0);
        for (int i = 1; i <= n; ++i) {
            const float f = (float)i / n;
            const Vec2 p = bezierPoint(kB0, kB1, kB2, kB3, mix(u0, u, f));
            drawThickLine(fb, prev, p, 3.0f + 15.0f * f, withAlpha(pal.cube, ir(10 + 90 * f)));
            prev = p;
        }
    }
}

// ---------------------------------------------------------------- beat 2: level cards
// A tiny diorama of what each level introduced, standing on the card's own ground line.
void drawCardScene(Framebuffer& fb, int i, int x, int y, int w, float t, const Palette& pal) {
    const int gy = y;  // ground line
    fb.fillRect(x, gy, w, 2, withAlpha(pal.neon, 200));
    const float fx = (float)x, fg = (float)gy;
    // Miniature player (drawCube's glow is sized for the full-scale cube).
    fb.fillRect(x + 4, gy - 16, 16, 16, rgb(8, 24, 14));
    fb.fillRect(x + 6, gy - 14, 12, 12, pal.cube);
    fb.fillRect(x + 8, gy - 11, 2, 3, rgb(8, 24, 14));
    fb.fillRect(x + 14, gy - 11, 2, 3, rgb(8, 24, 14));
    fb.fillRect(x + 9, gy - 6, 6, 1, rgb(8, 24, 14));
    switch (i % 3) {
        case 0:
            for (int k = 0; k < 3; ++k) drawSpike(fb, fx + w * 0.45f + k * 17.0f, fg, 16, 20, pal);
            drawSpike(fb, fx + w * 0.86f, fg, 16, 20, pal);
            break;
        case 1:
            drawSaw(fb, Vec2(fx + w * 0.48f, fg - 20), 15, t * 3.0f, pal);
            drawSpike(fb, fx + w * 0.72f, fg, 16, 20, pal);
            drawSaw(fb, Vec2(fx + w * 0.90f, fg - 16), 11, -t * 4.0f, pal);
            break;
        default:
            drawOrb(fb, Vec2(fx + w * 0.42f, fg - 24), 6, 0.5f + 0.5f * std::sin(t * 6), pal);
            drawSaw(fb, Vec2(fx + w * 0.64f, fg - 14), 12, t * 4.0f, pal);
            for (int k = 0; k < 2; ++k) drawSpike(fb, fx + w * 0.82f + k * 17.0f, fg, 16, 20, pal);
            break;
    }
}

void drawCards(Framebuffer& fb, float t, const FinaleStats& st, const Palette& pal) {
    const int n = std::min<int>(4, (int)st.levelNames.size());
    if (n == 0 || t < 2.6f || t > 6.5f) return;
    const int gap = 25, w = std::min(260, (880 - gap * (n - 1)) / n), h = 160;
    const int left = (W - (n * w + gap * (n - 1))) / 2, top = 190;
    const float stagger = 2.1f / n;
    const Color accents[3] = {pal.neon, pal.accent, pal.neon2};

    for (int i = 0; i < n; ++i) {
        const float tin = 2.7f + stagger * i;
        const float out = easeInCubic(seg(t, 5.6f + 0.08f * i, 6.1f + 0.08f * i));
        if (t < tin || out >= 1) continue;
        const float tStamp = tin + 1.0f, landed = tStamp + 0.18f;
        const float shake = t > landed ? 5.0f * std::exp(-(t - landed) * 14) * std::cos((t - landed) * 60) : 0;
        const int x = left + i * (w + gap) + ir((1 - easeOutBack(seg(t, tin, tin + 0.6f))) * 1000);
        const int y = top + ir(out * 420 + shake);
        const Color acc = accents[i % 3];

        fb.fillRect(x + 7, y + 7, w, h, withAlpha(kInk, 150));  // hard offset shadow
        fillRoundedRect(fb, x, y, w, h, 6, rgba(10, 12, 40, 240));
        drawRoundedRect(fb, x, y, w, h, 6, acc, 2);

        char buf[32];
        std::snprintf(buf, sizeof buf, "LEVEL %02d", i + 1);
        drawText(fb, x + 14, y + 12, buf, 2, acc);
        std::string name = st.levelNames[i];
        const int ns = textWidth(name, 3) <= w - 28 ? 3 : 2;
        while (name.size() > 1 && textWidth(name, ns) > w - 28) name.pop_back();
        drawText(fb, x + 14, y + 34, name, ns, kWhite);
        drawCardScene(fb, i, x + 14, y + 104, w - 28, t, pal);

        const float best = i < (int)st.levelTimes.size() ? st.levelTimes[i] : 0;
        const float cnt = easeOutCubic(seg(t, tin + 0.45f, tin + 1.0f));
        drawText(fb, x + 14, y + 115, "BEST", 1, kDim);
        drawText(fb, x + 14, y + 127, fmtTime(best * cnt), 3, kWhite);

        // Rubber stamp: bitmap text rotated and scaled by a Mat3, slammed onto the card.
        const float p = seg(t, tStamp, landed);
        if (p > 0) {
            const float sc = mix(2.8f, 1.0f, easeOutCubic(p));
            const Vec2 c((float)x + w - 68, (float)y + 134);
            const std::string word = "CLEARED";
            const float tw = (float)textWidth(word, 1), ts = 2.0f;
            const Mat3 M = Mat3::translate(c.x, c.y) * Mat3::rotate(-0.21f) * Mat3::scale(sc, sc);
            const float bw = tw * ts / 2 + 9, bh = 7 * ts / 2 + 7;
            Polygon box{M * Vec2(-bw, -bh), M * Vec2(bw, -bh), M * Vec2(bw, bh), M * Vec2(-bw, bh)};
            const Color sc1 = fade(pal.cube, p);
            fillPolygon(fb, box, fade(rgba(10, 12, 40, 235), p));
            drawPolygonThick(fb, box, 2.5f * sc, sc1, true);
            drawTextM(fb, M * Mat3::scale(ts, ts) * Mat3::translate(-tw / 2, -3.5f), word, sc1);
            const float ring = seg(t, landed, landed + 0.35f);  // impact ring (midpoint circle)
            if (ring > 0 && ring < 1)
                drawCircleThick(fb, ir(c.x), ir(c.y), ir(30 + 70 * easeOutCubic(ring)), 2,
                                withAlpha(pal.cube, ir(170 * (1 - ring))));
        }
    }

    // Typed caption above the cube.
    const std::string cap = "ALL LEVELS CLEARED";
    const float a = 1 - seg(t, 5.4f, 5.8f);
    if (a > 0) {
        const int shown = std::min<int>((int)cap.size(), (int)(seg(t, 2.6f, 3.4f) * cap.size() + 0.5f));
        const int cx = W / 2 - textWidth(cap, 3) / 2;
        drawTextShadow(fb, cx, 40, cap.substr(0, shown), 3, fade(kWhite, a), fade(kInk, a), 3);
        if (shown < (int)cap.size() || std::fmod(t, 0.5f) < 0.25f)
            fb.fillRect(cx + textWidth(cap.substr(0, shown), 3) + 4, 40, 12, 21, fade(pal.neon, a));
    }
}

// ---------------------------------------------------------------- beat 3: title
void drawTitle(Framebuffer& fb, float t, const Palette& pal) {
    const float t0 = 5.8f;
    if (t < t0) return;
    const std::string text = "YOU BEAT PULSERUN";
    const Bits b = rasterText(text);
    const float adv = (float)textWidth("AA", 1) - (float)textWidth("A", 1);  // glyph advance
    const float m = settle(t);
    const float s = mix(8.0f, 6.0f, m);
    const float ox = W / 2.0f - b.w * s / 2, oy = mix(120.0f, 16.0f, m);
    const bool assembled = t > t0 + 1.75f;
    const bool crisp = assembled && (m <= 0 || m >= 1);
    const int is = ir(s), iox = ir(ox), ioy = ir(oy);

    // Soft glow under the settled title (same trick as the menu logo).
    if (crisp) {
        const float ga = seg(t, t0 + 1.75f, t0 + 2.1f);
        for (int g = 3; g >= 1; --g) {
            drawText(fb, iox - g * 2, ioy, text, is, withAlpha(pal.neon, ir(13 * ga)));
            drawText(fb, iox + g * 2, ioy, text, is, withAlpha(pal.neon2, ir(13 * ga)));
        }
    }
    // A highlight sweeps across the lettering every few seconds.
    const float sweep = std::fmod(t - (t0 + 1.9f) + 60.0f, 5.0f) / 0.9f * (b.w + 24) - 12;

    for (int pass = 0; pass < 2; ++pass)  // 0 = drop shadow, 1 = lettering
        for (int py = 0; py < 7; ++py)
            for (int px = 0; px < b.w; ++px) {
                if (!b.at(px, py)) continue;
                const int ch = (int)(px / adv);
                Color col = ch < 8 ? kWhite : (ch < 14 ? pal.neon : pal.neon2);
                if (assembled) col = lerpColor(col, rgb(255, 255, 255), 0.85f * clamp01(1 - std::fabs(px - py - sweep) / 5.0f));
                if (pass == 0) col = kInk;
                const float off = pass == 0 ? std::max(3.0f, s / 2) : 0.0f;
                if (crisp) {
                    fb.fillRect(iox + px * is + (int)off, ioy + py * is + (int)off, is, is, col);
                    continue;
                }
                // Each block flies in from a scattered start, tumbling, left to right.
                const int id = py * 131 + px;
                const float delay = (float)px / b.w * 0.95f + h01(id, 21) * 0.3f;
                const float p = easeOutCubic(seg(t, t0 + delay, t0 + delay + 0.5f));
                if (p <= 0) continue;
                const Vec2 target(ox + (px + 0.5f) * s + off, oy + (py + 0.5f) * s + off);
                const float ang = h01(id, 22) * 2 * kPi, dist = 260 + h01(id, 23) * 420;
                const Vec2 pos = target + Vec2(std::cos(ang), std::sin(ang) * 0.7f) * (dist * (1 - p));
                const float half = s / 2 * mix(0.3f, 1.0f, p);
                const Mat3 M = Mat3::translate(pos.x, pos.y) * Mat3::rotate((1 - p) * (h01(id, 24) - 0.5f) * 9.0f);
                const Polygon q{M * Vec2(-half, -half), M * Vec2(half, -half), M * Vec2(half, half), M * Vec2(-half, half)};
                fillPolygon(fb, q, fade(col, pass == 0 ? p * p : 0.35f + 0.65f * p));
            }
}

// ---------------------------------------------------------------- beat 4: fireworks
// One shell: a rocket climbing a wobbly Catmull-Rom path, then a burst of sparks (lines +
// midpoint circles), an expanding ring and spinning polygon shards clipped to `clip`.
void drawShell(Framebuffer& fb, int seed, float age, Vec2 from, Vec2 apex, Color col, float gain, float reach,
               const ClipRect& clip) {
    const float rise = 0.6f, life = 1.5f;
    if (age < 0 || age > rise + life) return;
    if (age < rise) {
        const Vec2 d = apex - from;
        const float wob = (h01(seed, 31) - 0.5f) * 70;
        const std::vector<Vec2> path = sampleCatmullRom(
            {from, from + d * 0.33f + Vec2(wob, 0), from + d * 0.66f + Vec2(-wob * 0.6f, 0), apex}, 12);
        const float p = easeOutCubic(age / rise);
        const int n = (int)path.size(), head = std::min(n - 1, (int)(p * (n - 1)));
        for (int i = std::max(0, head - 9); i < head; ++i) {
            const float f = 1 - (float)(head - i) / 10.0f;
            double x0 = path[i].x, y0 = path[i].y, x1 = path[i + 1].x, y1 = path[i + 1].y;
            if (clipLineCohenSutherland(x0, y0, x1, y1, clip))
                drawThickLine(fb, Vec2((float)x0, (float)y0), Vec2((float)x1, (float)y1), 1 + 2 * f,
                              withAlpha(col, ir(200 * f * gain)));
        }
        if (path[head].y < clip.ymax) fillCircle(fb, ir(path[head].x), ir(path[head].y), 2, withAlpha(kWhite, ir(255 * gain)));
        return;
    }
    const float b = age - rise, k = b / life;
    const float a = std::pow(1 - k, 1.4f) * gain;
    auto sparkAt = [&](int i, float bb) {
        const float ang = 2 * kPi * (i + h01(seed, 40 + i) * 0.8f) / 22.0f;
        const float sp = reach * (0.55f + 0.45f * h01(seed, 70 + i));
        const float d = sp * easeOutCubic(clamp01(bb / life));
        return apex + Vec2(std::cos(ang), std::sin(ang)) * d + Vec2(0, 34 * bb * bb);
    };
    if (b < 0.16f) fillCircle(fb, ir(apex.x), ir(apex.y), ir(14 * (1 - b / 0.16f)), withAlpha(kWhite, ir(200 * gain)));
    if (b < 0.7f)
        drawCircle(fb, ir(apex.x), ir(apex.y), ir(reach * 1.1f * easeOutCubic(b / 0.7f)),
                   withAlpha(col, ir(150 * (1 - b / 0.7f) * gain)));
    for (int i = 0; i < 22; ++i) {
        const Vec2 p1 = sparkAt(i, b), p0 = sparkAt(i, std::max(0.0f, b - 0.09f));
        double x0 = p0.x, y0 = p0.y, x1 = p1.x, y1 = p1.y;
        if (!clipLineCohenSutherland(x0, y0, x1, y1, clip)) continue;
        drawThickLine(fb, Vec2((float)x0, (float)y0), Vec2((float)x1, (float)y1), 2.0f, withAlpha(col, ir(170 * a)));
        if (p1.y < clip.ymax - 3 && (i % 2 == 0 || k < 0.6f))
            fillCircle(fb, ir(p1.x), ir(p1.y), 2, withAlpha(lerpColor(kWhite, col, k), ir(255 * a)));
    }
    for (int i = 0; i < 6; ++i) {
        const float ang = 2 * kPi * h01(seed, 100 + i), sp = reach * (0.3f + 0.5f * h01(seed, 110 + i));
        const Vec2 c = apex + Vec2(std::cos(ang), std::sin(ang)) * (sp * easeOutCubic(k)) + Vec2(0, 60 * b * b);
        const float sz = 4 + 5 * h01(seed, 120 + i);
        const Mat3 M = Mat3::translate(c.x, c.y) * Mat3::rotate(ang + b * (4 + 6 * h01(seed, 130 + i)));
        const Polygon shard{M * Vec2(0, -sz), M * Vec2(sz * 0.9f, sz * 0.7f), M * Vec2(-sz * 0.8f, sz * 0.6f)};
        fillPolygonClipped(fb, shard, clip, withAlpha(col, ir(210 * a)));
    }
}

void drawFireworks(Framebuffer& fb, float t, const Palette& pal) {
    const Color cols[4] = {pal.neon, pal.neon2, pal.accent, pal.cube};
    const ClipRect full{0, 0, (double)W - 1, (double)H - 1};
    // Celebration salvo, first shell on the second heartbeat.
    for (int k = 0; k < 10; ++k) {
        const float age = t - (kThump2 + 0.27f * k);
        const Vec2 apex(110 + h01(k, 201) * 740, 70 + h01(k, 202) * 170);
        drawShell(fb, 500 + k, age, Vec2(apex.x + (h01(k, 203) - 0.5f) * 200, (float)H + 10), apex, cols[k % 4], 1.0f,
                  95.0f, full);
    }
    // Idle: a quiet shell now and then, kept inside the header band by clipping.
    if (t >= kIntro) {
        const ClipRect band{0, 0, (double)W - 1, 150};
        const float period = 1.9f;
        const int j = (int)((t - kIntro) / period);
        for (int k = std::max(0, j - 1); k <= j; ++k) {
            const float age = t - (kIntro + k * period);
            const bool leftSide = (k % 2) == 0;  // flank the title, never behind the stats or buttons
            const Vec2 apex(leftSide ? 40 + h01(k, 211) * 130 : 790 + h01(k, 211) * 130, 44 + h01(k, 212) * 40);
            drawShell(fb, 900 + k, age, Vec2(apex.x + (h01(k, 213) - 0.5f) * 60, 150), apex, cols[(k * 3 + 1) % 4], 0.7f,
                      48.0f, band);
        }
    }
}

// ---------------------------------------------------------------- beat 5: stats + credits
void drawStatsPanel(Framebuffer& fb, float t, const FinaleStats& st, const Palette& pal) {
    const float in = easeOutCubic(seg(t, 9.9f, 10.6f));
    if (in <= 0) return;
    const int rows = std::min<int>(4, (int)st.levelNames.size());
    const int w = 560, h = 84 + rows * 34 + (rows ? 8 : -10);
    const int x = 40 - ir((1 - in) * 640), y = 168;

    fb.fillRect(x + 7, y + 7, w, h, withAlpha(kInk, 150));
    fillRoundedRect(fb, x, y, w, h, 6, rgba(10, 12, 40, 236));
    drawRoundedRect(fb, x, y, w, h, 6, pal.neon, 2);

    const float cnt = easeOutCubic(seg(t, 10.5f, 11.4f));
    const Color diffCol = st.difficulty == "HARD" ? pal.hazard : (st.difficulty == "EASY" ? pal.cube : pal.accent);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d", ir(st.totalAttempts * cnt));
    const std::string labels[3] = {"DIFFICULTY", "ATTEMPTS", "TOTAL TIME"};
    const std::string values[3] = {st.difficulty.empty() ? "NORMAL" : st.difficulty, buf, fmtTime(st.totalTime * cnt)};
    const Color vcols[3] = {diffCol, kWhite, kWhite};
    for (int i = 0; i < 3; ++i) {
        const int cx = x + 18 + i * 184;
        drawText(fb, cx, y + 14, labels[i], 2, kDim);
        drawText(fb, cx, y + 36, values[i], 3, vcols[i]);
        if (i) fb.fillRect(cx - 14, y + 14, 1, 43, withAlpha(pal.neon, 70));
    }
    if (rows == 0) return;
    drawDashedLine(fb, x + 16, y + 70, x + w - 16, y + 70, withAlpha(pal.neon, 150), 6, 5);

    float longest = 0.001f;
    for (int i = 0; i < rows && i < (int)st.levelTimes.size(); ++i) longest = std::max(longest, st.levelTimes[i]);
    const Color accents[3] = {pal.neon, pal.accent, pal.neon2};
    for (int i = 0; i < rows; ++i) {
        const float rp = easeOutCubic(seg(t, 10.7f + 0.16f * i, 11.3f + 0.16f * i));
        if (rp <= 0) continue;
        const int rx = x - ir((1 - rp) * 26), ry = y + 84 + i * 34;
        const Color acc = accents[i % 3];
        const float best = i < (int)st.levelTimes.size() ? st.levelTimes[i] : 0;
        const int tries = i < (int)st.levelAttempts.size() ? st.levelAttempts[i] : 0;
        std::snprintf(buf, sizeof buf, "%02d", i + 1);
        drawText(fb, rx + 18, ry, buf, 2, fade(acc, rp));
        std::string name = st.levelNames[i];
        if (name.size() > 12) name = name.substr(0, 12);
        drawText(fb, rx + 54, ry, name, 2, fade(kWhite, rp));
        // Bar: this level's best time against the slowest one.
        const int bx = rx + 212, bw = 128;
        fb.fillRect(bx, ry + 3, bw, 8, fade(rgba(40, 46, 110, 255), rp));
        fb.fillRect(bx, ry + 3, ir(bw * rp * best / longest), 8, fade(acc, rp));
        textRight(fb, rx + 424, ry, fmtTime(best * rp), 2, fade(kWhite, rp));
        std::snprintf(buf, sizeof buf, "%d %s", tries, tries == 1 ? "TRY" : "TRIES");
        textRight(fb, rx + w - 18, ry, buf, 2, fade(kDim, rp));
    }
}

// Six small live demonstrations, one per algorithm named in the credits.
void drawAlgoIcon(Framebuffer& fb, int kind, int bx, int by, float tt, Color c, float a) {
    const Color hi = fade(c, a), lo = fade(c, a * 0.55f);
    const float fx = (float)bx, fy = (float)by;
    switch (kind) {
        case 0: {  // Bresenham: the pixels of a swinging line, magnified 3x
            const int k = ir(3.5f + 3.5f * std::sin(tt * 1.3f));
            for (const IPoint& p : bresenhamPoints(0, 7 - k, 9, k)) fb.fillRect(bx + p.x * 3, by + p.y * 3, 3, 3, hi);
            break;
        }
        case 1:  // midpoint circle with an arc sweeping round it
            drawCircle(fb, bx + 15, by + 11, 11, lo);
            drawArcThick(fb, bx + 15, by + 11, 11, tt * 110, tt * 110 + 130, 3, hi);
            break;
        case 2: {  // scanline fill creeping down a concave star
            const Polygon star = starPolygon(Vec2(fx + 15, fy + 11), 13, 5.5f, 5, -kPi / 2);
            const float scan = std::fmod(tt * 0.45f, 1.3f);
            fillPolygonClipped(fb, star, ClipRect{fx, fy - 3, fx + 30, fy - 3 + 28.0 * clamp01(scan)}, fade(c, a * 0.75f));
            drawPolygon(fb, star, hi);
            break;
        }
        case 3: {  // Cohen-Sutherland: only the part of the line inside the window is bright
            const ClipRect r{fx + 6, fy + 4, fx + 24, fy + 18};
            drawPolygon(fb, makeRect(fx + 6, fy + 4, 18, 14), lo);
            const Vec2 d = rotate(Vec2(17, 0), tt * 0.9f), m(fx + 15, fy + 11);
            drawLineBresenham(fb, ir(m.x - d.x), ir(m.y - d.y), ir(m.x + d.x), ir(m.y + d.y), lo);
            drawLineClipped(fb, m.x - d.x, m.y - d.y, m.x + d.x, m.y + d.y, r, hi);
            drawLineClipped(fb, m.x - d.x, m.y - d.y + 1, m.x + d.x, m.y + d.y + 1, r, hi);
            break;
        }
        case 4: {  // cubic Bezier and its control polygon
            const float wv = 6 * std::sin(tt * 1.5f);
            const Vec2 p0(fx, fy + 20), p1(fx + 8, fy - 6 + wv), p2(fx + 22, fy + 28 - wv), p3(fx + 30, fy + 2);
            drawPolygon(fb, Polygon{p0, p1, p2, p3}, lo, false);
            drawBezier(fb, p0, p1, p2, p3, 2.0f, hi, 24);
            break;
        }
        default: {  // Catmull-Rom spline threading its control points
            std::vector<Vec2> pts;
            for (int i = 0; i < 5; ++i)
                pts.push_back(Vec2(fx + i * 7.5f, fy + 11 + ((i % 2) ? -7.0f : 7.0f) * std::sin(tt * 1.4f + i * 0.5f)));
            drawPolylineThick(fb, sampleCatmullRom(pts, 8), 2.0f, hi);
            for (const Vec2& p : pts) fillCircle(fb, ir(p.x), ir(p.y), 2, fade(kWhite, a));
            break;
        }
    }
}

void drawCredits(Framebuffer& fb, float t, const Palette& pal) {
    const float t0 = 10.8f;
    const float in = easeOutCubic(seg(t, t0, t0 + 0.7f));
    if (in <= 0) return;
    const int y0 = 424 + ir((1 - in) * 140);
    fb.fillRect(0, y0 - 10, W, H - (y0 - 10), withAlpha(kInk, 150));
    drawDashedLine(fb, 40, y0 - 10, W - 40, y0 - 10, withAlpha(pal.neon, 170), 10, 6);

    // Two pages that take turns in the same band.
    const float pageLen = 6.0f, local = std::fmod(t - t0, pageLen);
    const int page = (int)((t - t0) / pageLen) % 2;
    const float a = std::min(seg(local, 0.0f, 0.45f), 1 - seg(local, pageLen - 0.45f, pageLen));
    const int slide = ir((1 - easeOutCubic(seg(local, 0.0f, 0.45f))) * 28);
    const int colX[3] = {172, 480, 788};
    const int rowY[2] = {y0 + 34, y0 + 70};

    if (page == 0) {
        drawTextCentered(fb, W / 2 + slide, y0 + 2, "MADE FROM SCRATCH - EVERY PIXEL BY OUR OWN ALGORITHMS", 2, fade(pal.neon, a));
        const char* names[6] = {"BRESENHAM", "MIDPOINT CIRCLES", "SCANLINE FILL", "COHEN-SUTHERLAND", "BEZIER", "CATMULL-ROM"};
        for (int i = 0; i < 6; ++i) {
            const int tw = textWidth(names[i], 2), total = 30 + 12 + tw;
            const int x = colX[i % 3] - total / 2 + slide, y = rowY[i / 3];
            drawAlgoIcon(fb, i, x, y - 4, t + i, (i % 2) ? pal.neon2 : pal.neon, a);
            drawText(fb, x + 42, y, names[i], 2, fade(kWhite, a));
        }
    } else {
        drawTextCentered(fb, W / 2 + slide, y0 + 2, "BUILT BY", 2, fade(pal.neon2, a));
        const char* team[6] = {"WACHIRA NJOROGE", "MATHENGE GITAHI", "STACY AKINYI",
                               "MARIE HOPPE SOLINI", "DIANA MUCHEKE", "ANISE WAMBUI KIMANI"};
        for (int i = 0; i < 6; ++i) {
            const int tw = textWidth(team[i], 2);
            const int x = colX[i % 3] - tw / 2 + slide + 8, y = rowY[i / 3];
            const Polygon gem = regularPolygon(Vec2((float)x - 14, (float)y + 7), 5, 4, t * 1.2f + i);
            fillPolygon(fb, gem, fade(pal.cube, a));
            drawText(fb, x, y, team[i], 2, fade(kWhite, a));
        }
    }
}

// ---------------------------------------------------------------- world
void drawWorld(Framebuffer& fb, float t, const Palette& pal) {
    const float down = easeInOutCubic(seg(t, 2.3f, 3.7f));  // the level sinks away and powers down
    const int groundY = 430 + ir(135 * down);
    const float scroll = 2400 + 700 * easeOutCubic(seg(t, 0.0f, 2.8f)) + 10 * t;
    drawBackdrop(fb, scroll, groundY, pal);
    if (groundY < H) drawGround(fb, groundY, scroll, pal);
    if (down > 0) {
        fb.fillRect(0, 0, W, H, withAlpha(kInk, ir(150 * down)));
        // Stars come out as the lights go down.
        for (int i = 0; i < 46; ++i) {
            const int x = ir(h01(i, 301) * W), y = ir(h01(i, 302) * 400);
            const float tw = 0.55f + 0.45f * std::sin(t * (0.8f + h01(i, 303) * 1.6f) + i);
            const Color c = withAlpha(kWhite, ir(150 * down * tw));
            fb.fillRect(x, y, 2, 2, c);
            if (i % 5 == 0) { fb.fillRect(x - 2, y, 6, 2, fade(c, 0.5f)); fb.fillRect(x, y - 2, 2, 6, fade(c, 0.5f)); }
        }
    }
}

}  // namespace

float finaleIntroDuration() { return kIntro; }

void finaleButtonArea(int& x, int& y, int& w, int& h) {
    x = kBtnX;
    y = kBtnY;
    w = kBtnW;
    h = kBtnH;
}

void drawFinale(Framebuffer& fb, float t, const FinaleStats& stats, const Palette& pal) {
    if (t < 0) t = 0;
    drawWorld(fb, t, pal);
    drawEscape(fb, t, pal);
    drawFireworks(fb, t, pal);
    drawCards(fb, t, stats, pal);
    drawHeartbeat(fb, t, pal);
    drawTitle(fb, t, pal);

    const CubePose cube = cubePose(t);
    if (t >= 0.3f) drawCube(fb, cube.pos, cube.size, cube.angle, pal);

    drawStatsPanel(fb, t, stats, pal);
    drawCredits(fb, t, pal);

    // Prompt above the button stack.
    const float pa = seg(t, 11.3f, 11.8f);
    if (pa > 0) {
        drawTextCentered(fb, kBtnX + kBtnW / 2, kBtnY - 28, "RUN IT BACK?", 2, fade(kDim, pa));
    }

    // Cinematic bars for the slow-motion escape.
    const int bar = ir(40 * easeOutCubic(seg(t, 0.0f, 0.4f)) * (1 - easeInOutCubic(seg(t, 2.2f, 2.9f))));
    if (bar > 0) {
        fb.fillRect(0, 0, W, bar, rgb(0, 0, 0));
        fb.fillRect(0, H - bar, W, bar, rgb(0, 0, 0));
    }
}
