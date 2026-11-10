#include "game/progress.h"
#include <algorithm>
#include <cstdio>
#include <sstream>

LevelRecord& Progress::at(int n) {
    if (n < 1) n = 1;
    if ((int)levels.size() < n) levels.resize(n);
    return levels[n - 1];
}

bool Progress::recordDeath(int n, float p) {
    LevelRecord& r = at(n);
    ++r.attempts;
    if (p > r.best + 1e-4f) {
        r.best = p;
        return true;
    }
    return false;
}

bool Progress::recordComplete(int n, float seconds) {
    LevelRecord& r = at(n);
    ++r.attempts;
    r.best = 1.0f;
    r.completed = true;
    if (r.bestTime <= 0.0f || seconds < r.bestTime) {
        r.bestTime = seconds;
        return true;
    }
    return false;
}

// Format: "pulserun 1" header, then one "level N best time attempts completed" line per level.
std::string Progress::serialize() const {
    std::ostringstream o;
    o << "pulserun 1\n";
    o << "hitboxes " << (hitboxes ? 1 : 0) << "\n";
    o << "hint " << (seenHint ? 1 : 0) << "\n";
    for (size_t i = 0; i < levels.size(); ++i) {
        const LevelRecord& r = levels[i];
        char buf[96];
        std::snprintf(buf, sizeof buf, "level %d %.4f %.3f %d %d\n", (int)i + 1, r.best, r.bestTime, r.attempts,
                      r.completed ? 1 : 0);
        o << buf;
    }
    return o.str();
}

Progress Progress::parse(const std::string& text) {
    Progress p;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string key;
        ls >> key;
        if (key == "hitboxes") {
            int v = 0;
            if (ls >> v) p.hitboxes = v != 0;
        } else if (key == "hint") {
            int v = 0;
            if (ls >> v) p.seenHint = v != 0;
        } else if (key == "level") {
            int n = 0, attempts = 0, done = 0;
            float best = 0, t = 0;
            if (ls >> n >> best >> t >> attempts >> done && n >= 1 && n <= 99) {
                LevelRecord& r = p.at(n);
                r.best = std::clamp(best, 0.0f, 1.0f);
                r.bestTime = std::max(0.0f, t);
                r.attempts = std::max(0, attempts);
                r.completed = done != 0;
            }
        }
    }
    return p;
}
