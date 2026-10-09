#pragma once
#include "game.h"
#include "combo_core.h"
#include "combo_profiles.h"
namespace combos {
struct Settings {bool enabled=false,confirmed=false;int holdKey=0;int keys[3]={};};
inline Settings settings[count];
void InitializeSettings();
// Returns true while this module owns the input lane, even if blocked/no key was sent.
bool Run(const Frame& frame);
void Cancel(const char* reason);
void Reset();
const char* Status();
const char* LastOutcome();
void DrawSettings(int profile,bool english);
}
