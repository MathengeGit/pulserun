#pragma once
#include "core/color.h"
#include "game/obstacle.h"
#include <string>
#include <vector>

// A level loaded from a plain-text file in levels/. Format (one item per line, '#' comments):
//
//   name PRIMER            display name (rest of the line)
//   number 1               level number shown in the HUD
//   difficulty 0           0 easy, 1 normal, 2 hard (level-select face)
//   width 10000            level length in px
//   ground_y 500           world y of the ground surface
//   speed 420              run speed in px/s
//   bg_color 0x10164a      sky colour (top of the gradient)
//
//   BLOCK   x y w h        top-left + size; tiled with square blocks of size min(w, h)
//   SPIKE   x y [w h]      top-left of the spike's bounding box (default 50 x 50)
//   SAW     x y radius=R speed=S   centre, radius (default 30), spin in turns/s (default 1)
//   ORB     x y [radius=R] centre of a jump orb (default radius 16)
//   PAD     x y [w h]      top-left of a jump pad (default 50 x 12)
//   PORTAL  x y [target=N] centre of the exit portal; N = next level (0 = none)
//
// Numbers may also be given as key=value (w=, h=, radius=, speed=, target=).
struct Level {
    std::string name = "UNTITLED";
    int number = 1;
    int difficulty = 0;
    float width = 10000.0f;
    float groundY = 500.0f;
    float speed = 420.0f;
    bool hasBg = false;
    Color bg{16, 22, 74, 255};
    std::vector<Obstacle> obstacles;  // sorted by x

    // Where the run ends: the portal's x if there is one, else the level width.
    float endX() const;
    float startX() const { return 120.0f; }
    int nextLevel() const;  // the portal's target, 0 if none
};

// Parse level text. On failure returns false and, if `error` is given, a message such as
// "line 7: unknown object 'SPIKES'".
bool parseLevel(const std::string& text, Level& out, std::string* error = nullptr);
bool loadLevel(const std::string& path, Level& out, std::string* error = nullptr);
