#pragma once
#include "game.h"
#include <d3d11.h>
namespace hud {
struct ReadProbe { int buffLists=0,buffUnknown=0,buffEffects=0; int heroes=0, inventories=0, items=0, abilities=0, mappedItems=0, mappedAbilities=0, aegis=0, aegisTimed=0, aegisEstimated=0, aegisFallback=0, inventorySlots=0, resolved=0, unmapped=0; };
extern ReadProbe readProbe;
void Initialize(ID3D11Device* device);
void Shutdown();
void UpdateAegis(Frame& frame);
void Draw(const Frame& frame);
void DrawWorldHero(const Frame& frame, const FrameUnit& hero);
}
