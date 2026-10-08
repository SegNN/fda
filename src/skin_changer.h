#pragma once
#include "cosmetic_core.h"
namespace skins {
struct Status {bool connected=false,enabled=false,cacheReady=false;size_t definitions=0,selections=0,pending=0;uint64_t account=0;std::string message;};
const cosmetic::Definition* Catalog(size_t& count);
void Poll();
Status GetStatus();
bool SetEnabled(bool on);
bool Refresh();
bool Select(uint32_t definition,uint32_t cls,uint32_t slot,uint32_t style);
bool Reset();
bool CanUnload();
void NotifyUnloadBlocked();
void Shutdown();
void DrawSettings(bool english,float scale=1.f);
}
