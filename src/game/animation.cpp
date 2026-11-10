#include "game/animation.h"
#include "math/mat3.h"
#include "raster/circle.h"
#include <algorithm>

void Animation::update(float dt) {
    if (frames.empty()) return;
    elapsed += dt;
    const int n = (int)frames.size();
    const int f = (int)(elapsed * fps);
    current = loop ? f % n : std::min(f, n - 1);
}

void Animation::draw(Framebuffer& fb, Vec2 pos) const {
    if (!frames.empty()) frames[current](fb, pos);
}

Animation makeDeathBurst(const Palette& pal, const ClipRect& viewport) {
    Animation a;
    a.fps = 15.0f;
    for (int i = 0; i < 5; ++i) {
        const float t = 0.1f + 0.15f * i;
        a.frames.push_back([pal, viewport, t](Framebuffer& fb, Vec2 p) { drawDeathBurst(fb, p, t, pal, viewport); });
    }
    return a;
}

Animation makePadSpring(const Palette& pal) {
    Animation a;
    a.fps = 20.0f;
    for (float squash : {1.0f, 0.55f, 0.25f, 0.7f})
        a.frames.push_back([pal, squash](Framebuffer& fb, Vec2 p) { drawPadCompressed(fb, p.x, p.y, squash, pal); });
    return a;
}

Animation makeOrbPop(const Palette& pal) {
    Animation a;
    a.fps = 20.0f;
    for (int i = 0; i < 4; ++i) {
        const int r = 18 + i * 9, alpha = 220 - i * 50;
        a.frames.push_back([pal, r, alpha](Framebuffer& fb, Vec2 p) {
            drawCircleThick(fb, (int)p.x, (int)p.y, r, 3, withAlpha(pal.accent, alpha));
        });
    }
    return a;
}

void Effects::spawn(Animation a, Vec2 world) { list_.push_back(Effect{std::move(a), world}); }

void Effects::update(float dt) {
    for (Effect& e : list_) e.anim.update(dt);
    list_.erase(std::remove_if(list_.begin(), list_.end(), [](const Effect& e) { return e.anim.finished(); }), list_.end());
}

void Effects::draw(Framebuffer& fb, const Mat3& view) const {
    for (const Effect& e : list_) e.anim.draw(fb, view * e.world);
}
