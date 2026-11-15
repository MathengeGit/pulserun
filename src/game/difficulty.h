#pragma once

// Difficulty modes. Every mode plays the same level files; what changes is how much the
// game forgives. HARD is the original tuning (the game as first shipped); NORMAL and EASY
// slow the world down and widen the timing windows. The numbers were tuned with the
// timing-window analyser (pulserun --analyse), not by feel: see README "Difficulty".
enum class Difficulty : int { Easy = 0, Normal = 1, Hard = 2 };
constexpr int kDifficultyCount = 3;

struct Rules {
    const char* name = "HARD";
    // World time runs at this fraction of real time. Jump arcs and level geometry are
    // unchanged (so every level stays beatable), but the player gets more time to react.
    float timeScale = 1.0f;
    // Spike hitbox: the triangle keeps the middle (1 - 2*spikeInset) of its base, and its tip
    // is lowered by the same fraction of its height.
    float spikeInset = 0.22f;
    float sawScale = 0.82f;    // saw hitbox radius as a fraction of the drawn radius
    float coyote = 0.0f;       // s after running off a ledge that a press still jumps
    float buffer = 0.10f;      // s a press made before landing (or before an orb) is remembered
    float blockGrace = 0.0f;   // px: clipping a block corner by this little nudges you up instead of killing
    bool checkpoints = false;  // respawn at the last checkpoint instead of the level start
};

inline Rules rulesFor(Difficulty d) {
    Rules r;  // HARD = the original game
    if (d == Difficulty::Normal) {
        r.name = "NORMAL";
        r.timeScale = 0.88f;
        r.spikeInset = 0.27f;
        r.sawScale = 0.74f;
        r.coyote = 0.06f;
        r.buffer = 0.14f;
        r.blockGrace = 8.0f;
    } else if (d == Difficulty::Easy) {
        r.name = "EASY";
        r.timeScale = 0.76f;
        r.spikeInset = 0.31f;
        r.sawScale = 0.66f;
        r.coyote = 0.10f;
        r.buffer = 0.18f;
        r.blockGrace = 14.0f;
        r.checkpoints = true;
    }
    return r;
}

inline const char* difficultyName(Difficulty d) { return rulesFor(d).name; }
