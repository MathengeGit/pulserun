#include "game/level.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

bool fail(std::string* error, int line, const std::string& msg) {
    if (error) *error = "line " + std::to_string(line) + ": " + msg;
    return false;
}

bool toFloat(const std::string& s, float& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtof(s.c_str(), &end);
    return end && *end == '\0';
}

bool typeFromName(const std::string& n, ObstacleType& t) {
    static const struct { const char* name; ObstacleType type; } kTypes[] = {
        {"BLOCK", ObstacleType::Block}, {"SPIKE", ObstacleType::Spike}, {"SAW", ObstacleType::Saw},
        {"ORB", ObstacleType::Orb},     {"PAD", ObstacleType::Pad},     {"PORTAL", ObstacleType::Portal}};
    for (const auto& k : kTypes)
        if (n == k.name) {
            t = k.type;
            return true;
        }
    return false;
}

void applyDefaults(Obstacle& o) {
    switch (o.type) {
    case ObstacleType::Spike: o.w = 50; o.h = 50; break;
    case ObstacleType::Saw: o.radius = 30; o.speed = 1; break;
    case ObstacleType::Orb: o.radius = 16; break;
    case ObstacleType::Pad: o.w = 50; o.h = 12; break;
    case ObstacleType::Portal: o.w = 44; o.h = 112; break;
    case ObstacleType::Block: break;
    }
}

}  // namespace

float Level::endX() const {
    for (const Obstacle& o : obstacles)
        if (o.type == ObstacleType::Portal) return o.x;
    return width;
}

int Level::nextLevel() const {
    for (const Obstacle& o : obstacles)
        if (o.type == ObstacleType::Portal) return o.target;
    return 0;
}

bool parseLevel(const std::string& text, Level& out, std::string* error) {
    Level lvl;
    std::istringstream in(text);
    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        const size_t hash = raw.find('#');
        if (hash != std::string::npos) raw.erase(hash);
        std::istringstream ls(raw);
        std::string key;
        if (!(ls >> key)) continue;  // blank or comment-only line

        if (key == "name") {
            std::string rest;
            std::getline(ls >> std::ws, rest);
            while (!rest.empty() && (rest.back() == '\r' || rest.back() == ' ')) rest.pop_back();
            if (rest.empty()) return fail(error, lineNo, "name needs a value");
            lvl.name = rest;
            continue;
        }
        if (key == "bg_color") {
            std::string v;
            ls >> v;
            char* end = nullptr;
            const unsigned long c = std::strtoul(v.c_str(), &end, 16);
            if (v.empty() || !end || *end) return fail(error, lineNo, "bg_color must be hex, e.g. 0x10164a");
            lvl.bg = rgb((int)((c >> 16) & 255), (int)((c >> 8) & 255), (int)(c & 255));
            lvl.hasBg = true;
            continue;
        }
        if (key == "width" || key == "ground_y" || key == "speed" || key == "number" || key == "difficulty") {
            std::string v;
            float f = 0;
            if (!(ls >> v) || !toFloat(v, f)) return fail(error, lineNo, key + " needs a number");
            if (key == "width") lvl.width = f;
            if (key == "ground_y") lvl.groundY = f;
            if (key == "speed") lvl.speed = f;
            if (key == "number") lvl.number = (int)f;
            if (key == "difficulty") lvl.difficulty = std::clamp((int)f, 0, 2);
            continue;
        }

        Obstacle o;
        if (!typeFromName(key, o.type)) return fail(error, lineNo, "unknown object '" + key + "'");
        applyDefaults(o);
        std::vector<float> nums;
        std::string tok;
        while (ls >> tok) {
            const size_t eq = tok.find('=');
            float f = 0;
            if (eq == std::string::npos) {
                if (!toFloat(tok, f)) return fail(error, lineNo, "bad number '" + tok + "'");
                nums.push_back(f);
                continue;
            }
            const std::string k = tok.substr(0, eq);
            if (!toFloat(tok.substr(eq + 1), f)) return fail(error, lineNo, "bad value in '" + tok + "'");
            if (k == "w") o.w = f;
            else if (k == "h") o.h = f;
            else if (k == "radius") o.radius = f;
            else if (k == "speed") o.speed = f;
            else if (k == "target") o.target = (int)f;
            else return fail(error, lineNo, "unknown option '" + k + "'");
        }
        if (nums.size() < 2) return fail(error, lineNo, std::string(obstacleName(o.type)) + " needs x and y");
        o.x = nums[0];
        o.y = nums[1];
        if (nums.size() >= 3) o.w = nums[2];
        if (nums.size() >= 4) o.h = nums[3];
        if (nums.size() > 4) return fail(error, lineNo, "too many numbers");
        if (o.type == ObstacleType::Block && (o.w <= 0 || o.h <= 0))
            return fail(error, lineNo, "BLOCK needs x y w h");
        lvl.obstacles.push_back(o);
    }
    std::stable_sort(lvl.obstacles.begin(), lvl.obstacles.end(),
                     [](const Obstacle& a, const Obstacle& b) { return a.x < b.x; });
    out = std::move(lvl);
    return true;
}

bool loadLevel(const std::string& path, Level& out, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return parseLevel(ss.str(), out, error);
}
