#pragma once
#include "game.h"
#include <d3d11.h>
#include <dxgi.h>
namespace topanchor {
struct Anchor { float x=0,y=0,w=0,h=0,confidence=0; };
struct Stats { int matched=0,jobs=0; float best=0; bool captureOk=false; };
extern Stats stats;
void Initialize(ID3D11Device* device);
void Shutdown();
void Update(IDXGISwapChain* chain,const Frame& frame);
bool Find(const FrameUnit& unit,Anchor& anchor);
}
