#pragma once
#include <string>

// One small text blob of saved progress.
//   browser: localStorage["pulserun.save"] (silently does nothing if storage is blocked)
//   desktop: pulserun_save.txt in the working directory
// setStorageEnabled(false) turns both into no-ops (tests and headless rendering).
std::string loadSaveText();
void writeSaveText(const std::string& text);
void setStorageEnabled(bool enabled);
