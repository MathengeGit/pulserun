#pragma once
#include "game/level.h"
#include <string>
#include <vector>

// Records the demo videos: drives the real Game with the solver's inputs, renders every
// 60 Hz step with our own rasteriser, and pipes the raw frames to ffmpeg for encoding
// (ffmpeg only compresses; it draws nothing). Writes into `dir`:
//   01_level_complete.mp4   finishing a level: portal, complete screen, on to the next level
//   02_easy.mp4 / 03_normal.mp4 / 04_hard.mp4   the same level on each difficulty
//   05_difficulty_compare.mp4   all three modes side by side, same real time
//   06_finale.mp4           the last level's finish and the game-beaten finale
//   pulserun_demo_reel.mp4  everything above, back to back
int runRecord(const std::vector<Level>& levels, const std::string& dir, const std::string& ffmpeg);
