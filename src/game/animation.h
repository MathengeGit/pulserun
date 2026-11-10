#pragma once
#include "core/framebuffer.h"
#include "game/sprites.h"
#include "math/mat3.h"
#include "math/vec2.h"
#include <functional>
#include <vector>

// Flip-book animation: each frame is a draw call at a position. update() advances by real
// time at `fps`; a non-looping animation holds its last frame once finished.
struct Animation {
    using Frame = std::function<void(Framebuffer&, Vec2)>;
    std::vector<Frame> frames;
    float fps = 12.0f;
    float elapsed = 0.0f;
    int current = 0;
    bool loop = false;

    void update(float dt);
    void draw(Framebuffer& fb, Vec2 pos) const;
    void restart() { elapsed = 0; current = 0; }
    bool finished() const { return !loop && elapsed >= duration(); }
    float duration() const { return frames.empty() ? 0.0f : frames.size() / fps; }
};

// ---- Animations used by the game ----
// Death: a 5-frame shock wave + shard burst at 15 fps (drawDeathBurst at t = 0.1 .. 0.7).
Animation makeDeathBurst(const Palette& pal, const ClipRect& viewport);
// Jump pad: 4 frames of spring compression and rebound at 20 fps. Draw at (centreX, surfaceY).
Animation makePadSpring(const Palette& pal);
// Orb pop: 4 frames of an expanding ring at 20 fps when an orb is used.
Animation makeOrbPop(const Palette& pal);

// Effects that play at a world position (the camera is applied when drawing).
struct Effect {
    Animation anim;
    Vec2 world;
};
class Effects {
public:
    void spawn(Animation a, Vec2 world);
    void update(float dt);
    void draw(Framebuffer& fb, const Mat3& worldToScreen) const;
    void clear() { list_.clear(); }
    bool empty() const { return list_.empty(); }

private:
    std::vector<Effect> list_;
};
