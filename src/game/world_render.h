#pragma once
#include "game/camera.h"
#include "game/sprites.h"
#include "game/world.h"

struct RenderOptions {
    bool hitboxes = false;  // overlay collision polygons and AABBs (toggle with H)
    bool drawPlayer = true;
};

// The default palette with the level's sky colour.
Palette levelPalette(const Level& level);

// Screen y of the ground for this camera.
float groundScreenY(const World& w, const Camera& cam);

// Draw the level and the player. Every world-space position goes through
// cam.world_to_screen() (a Mat3) before it reaches a raster call.
void drawWorld(Framebuffer& fb, const World& w, const Camera& cam, float alpha, const Palette& pal,
               const RenderOptions& opt = {});

// Collision overlay: obstacle hulls, broad-phase boxes and the player's hull.
void drawHitboxes(Framebuffer& fb, const World& w, const Mat3& view);
