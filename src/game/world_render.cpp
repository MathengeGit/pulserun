#include "game/world_render.h"
#include "game/animation.h"
#include "raster/circle.h"
#include "raster/curve.h"
#include "raster/line.h"
#include <algorithm>
#include <cmath>

Palette levelPalette(const Level& level) {
    Palette p = defaultPalette();
    if (level.hasBg) p.bgTop = level.bg;
    return p;
}

float groundScreenY(const World& w, const Camera& cam) {
    return (cam.world_to_screen() * Vec2(0, w.level ? w.level->groundY : 500)).y;
}

namespace {

Polygon toScreen(const Polygon& p, const Mat3& m) {
    Polygon out;
    out.reserve(p.size());
    for (const Vec2& v : p) out.push_back(m * v);
    return out;
}

void drawBlockRun(Framebuffer& fb, Vec2 s, float w, float h, const Palette& pal) {
    const float size = std::min(w, h);
    const int cols = std::max(1, (int)std::lround(w / size)), rows = std::max(1, (int)std::lround(h / size));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) drawBlock(fb, s.x + c * size, s.y + r * size, size, pal);
}

void drawObstacle(Framebuffer& fb, const World& w, size_t i, const Mat3& view, const Palette& pal, float t) {
    const Obstacle& o = w.level->obstacles[i];
    const Vec2 s = view * Vec2(o.x, o.y);
    switch (o.type) {
    case ObstacleType::Block: drawBlockRun(fb, s, o.w, o.h, pal); break;
    case ObstacleType::Spike: drawSpike(fb, s.x + o.w / 2, s.y + o.h, o.w, o.h, pal); break;
    case ObstacleType::Saw: drawSaw(fb, s, o.radius, o.sawAngle(t), pal); break;
    case ObstacleType::Orb:
        if (w.orbUsed[i]) drawCircleThick(fb, (int)s.x, (int)s.y, (int)o.radius, 2, withAlpha(pal.accent, 90));
        else drawOrb(fb, s, (int)o.radius, 0.5f + 0.5f * std::sin(t * 6.0f), pal);
        break;
    case ObstacleType::Pad: {
        // Spring compression plays for a moment after the pad fires.
        static Animation spring;
        if (spring.frames.empty()) spring = makePadSpring(pal);
        if (w.padAge[i] < spring.duration()) {
            spring.elapsed = w.padAge[i];
            spring.update(0);
            spring.draw(fb, Vec2(s.x + o.w / 2, s.y + o.h));
        } else {
            drawPad(fb, s.x + o.w / 2, s.y + o.h, pal);
        }
        break;
    }
    case ObstacleType::Portal: drawPortal(fb, s, o.w / 2, o.h / 2, pal.neon2); break;
    }
}

}  // namespace

void drawHitboxes(Framebuffer& fb, const World& w, const Mat3& view) {
    const Color hazard = rgb(255, 80, 80), solid = rgb(80, 230, 255), trigger = rgb(255, 230, 80);
    for (size_t i = 0; i < w.level->obstacles.size(); ++i) {
        const Obstacle& o = w.level->obstacles[i];
        const AABB b = o.box();
        const Vec2 a = view * Vec2(b.x0, b.y0), c = view * Vec2(b.x1, b.y1);
        if (c.x < 0 || a.x > fb.width()) continue;
        const Color col = o.deadly() ? hazard : (o.type == ObstacleType::Block ? solid : trigger);
        // Broad phase box (dashed), then the exact hull (solid).
        drawDashedLine(fb, (int)a.x, (int)a.y, (int)c.x, (int)a.y, withAlpha(col, 140), 4, 3);
        drawDashedLine(fb, (int)c.x, (int)a.y, (int)c.x, (int)c.y, withAlpha(col, 140), 4, 3);
        drawDashedLine(fb, (int)c.x, (int)c.y, (int)a.x, (int)c.y, withAlpha(col, 140), 4, 3);
        drawDashedLine(fb, (int)a.x, (int)c.y, (int)a.x, (int)a.y, withAlpha(col, 140), 4, 3);
        drawPolygon(fb, toScreen(o.hull(w.time), view), col);
    }
    drawPolygon(fb, toScreen(w.player.hull(2.0f), view), rgb(120, 255, 120));
    drawPolygon(fb, toScreen(w.player.hull(0.0f), view), withAlpha(rgb(120, 255, 120), 120));
}

void drawWorld(Framebuffer& fb, const World& w, const Camera& cam, float alpha, const Palette& pal,
               const RenderOptions& opt) {
    if (!w.level) return;
    const Mat3 view = cam.world_to_screen();
    const float gy = groundScreenY(w, cam);
    drawBackdrop(fb, cam.scroll_x, (int)std::min(gy, (float)fb.height() + 40), pal);
    drawGround(fb, (int)gy, cam.scroll_x, pal);

    const float t = w.time;
    const float left = cam.scroll_x - 300, right = cam.scroll_x + fb.width() + 300;
    for (size_t i = 0; i < w.level->obstacles.size(); ++i) {
        const Obstacle& o = w.level->obstacles[i];
        const AABB b = o.box();
        if (b.x1 < left || b.x0 > right) continue;
        drawObstacle(fb, w, i, view, pal, t);
    }

    if (opt.drawPlayer && !w.dead) {
        const Vec2 pos = w.player.prev + (w.player.pos - w.player.prev) * alpha;
        // Motion trail: a Catmull-Rom spline through recent positions, fading toward the tail.
        if (w.trail.size() >= 3) {
            std::vector<Vec2> pts;
            for (const Vec2& p : w.trail) pts.push_back(view * (p + Vec2(-Player::SIZE * 0.25f, 0)));
            pts.push_back(view * (pos + Vec2(-Player::SIZE * 0.25f, 0)));
            const auto curve = sampleCatmullRom(pts, 4);
            for (size_t i = 1; i < curve.size(); ++i) {
                const float k = (float)i / curve.size();
                drawThickLine(fb, curve[i - 1], curve[i], 2.0f + 10.0f * k, withAlpha(pal.cube, (int)(70 * k)));
            }
        }
        const float ang = w.player.prevAngle + (w.player.angle - w.player.prevAngle) * alpha;
        drawCube(fb, view * pos, 56, ang, pal);
    }
    if (opt.hitboxes) drawHitboxes(fb, w, view);
}
