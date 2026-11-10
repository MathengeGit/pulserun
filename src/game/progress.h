#pragma once
#include <string>
#include <vector>

// Per-level records shown on the level cards and the complete / death screens.
struct LevelRecord {
    float best = 0.0f;      // best progress 0..1
    float bestTime = 0.0f;  // fastest completion in seconds (0 = never completed)
    int attempts = 0;
    bool completed = false;
};

struct Progress {
    std::vector<LevelRecord> levels;
    bool hitboxes = false;  // settings
    bool seenHint = false;  // first-run "how to jump" hint already shown

    LevelRecord& at(int levelNumber);  // 1-based; grows as needed
    // Returns true if this run set a new best.
    bool recordDeath(int levelNumber, float progress);
    bool recordComplete(int levelNumber, float seconds);

    std::string serialize() const;
    static Progress parse(const std::string& text);  // tolerant: unknown or bad lines are skipped
};
