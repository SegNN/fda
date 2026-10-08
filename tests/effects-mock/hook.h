#pragma once
#include "game.h"
#include "imgui.h"

void InstallHooks();
void RequestUnload();

void DrawMenuWindow();

namespace view {
void Update();
bool W2S(const Vec3& world, ImVec2& out);
bool W2SRaw(const Vec3& world, ImVec2& out);
extern int W, H;
}
void DrawOverlay(const Frame& f);
void RunAutomation(const Frame& f);
void DrawKeybinds();
void RunAutoAccept();

// Read-only diagnostic snapshot, refreshed on the render thread.
namespace diagnostics {
struct Snapshot {
    bool frameOk = false;
    bool matrixOk = false;
    bool projectedLocal = false;
    bool projectedHero = false;
    int unitCount = 0;
    int heroCount = 0;
};
extern Snapshot snapshot;
void Update(const Frame& frame);
}
