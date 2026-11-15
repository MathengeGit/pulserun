#pragma once
#include "game/collision.h"
#include "game/difficulty.h"
#include "game/level.h"
#include "game/player.h"
#include <vector>

// What happened during one World::step (for sounds, effects and the screens).
struct WorldEvents {
    bool died = false, completed = false, jumped = false, orb = false, pad = false, landed = false;
    int killer = -1;
    int orbIndex = -1, padIndex = -1;  // which orb / pad fired
};

// One attempt at a level: the player, the clock and per-object state. Deterministic: the
// same inputs at the same steps always give the same run, which the solver relies on.
class World {
public:
    static constexpr float ORB_VEL = 640.0f;      // upward speed from a jump orb
    static constexpr float PAD_VEL = 820.0f;      // upward speed from a jump pad
    static constexpr float JUMP_BUFFER = 0.10f;   // a press this early before landing still jumps
    static constexpr int TRAIL = 14;              // trail samples kept for rendering

    World() = default;
    explicit World(const Level* level) { setLevel(level); }

    void setLevel(const Level* level);
    void setRules(const Rules& r) { rules = r; }
    void reset();
    // EASY checkpoint respawn: a fresh run standing on the ground at world x, orbs behind it
    // spent. Always the same state, so the tests can prove every checkpoint is safe.
    void spawnAt(float x);
    // Advance one fixed step of real time `dt`; the world itself advances dt * rules.timeScale.
    // `held` is the jump input state, `pressed` its rising edge.
    WorldEvents step(float dt, bool held, bool pressed);

    float progress() const;  // 0..1 along the level
    bool touchingUnusedOrb() const;

    const Level* level = nullptr;
    Rules rules;            // difficulty (default HARD = original tuning)
    Player player;
    float time = 0.0f;      // world seconds (drives physics and saw spin)
    float clock = 0.0f;     // real seconds played (shown as the run time)
    bool dead = false, complete = false;
    int killer = -1;
    int steps = 0;
    std::vector<char> orbUsed;
    std::vector<float> padAge;  // seconds since each pad fired (large = idle)
    std::vector<Vec2> trail;    // recent centres, newest last

private:
    void nearby(size_t& first, size_t& last) const;
    float buffer_ = 0.0f;
    float airborne_ = 99.0f;  // world seconds since the cube last stood on something (coyote time)
    int lastPad_ = -1;
    float maxExtent_ = 0.0f;
};
