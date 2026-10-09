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

// Read-only diagnostic snapshot, refreshed on the render thread.
namespace diagnostics {
struct HeroSample {
    char name[64]={}; Vec3 pos{}; ImVec2 foot{},head{};
    int items=0,abilities=0,buffs=0,inventorySlots=-1,resolved=0,unmapped=0,level=0;
    bool inventoryRead=false,buffsRead=false,buffsVisualRead=false,illusionRead=false,illusion=false,invisRead=false,stateRead=false,clockRead=false;
    uint64_t state=0;float clock=0;
    int shownAbilityCount=0;char abilityNames[6][80]={};int abilityLevels[6]={},abilityMaxima[6]={};
    uint64_t inventoryRaw0=0,inventoryRaw8=0;uint32_t inventoryParent=0;int inventoryProbeMask=0;
    uint32_t entityHandle=0;int ownerID=-1,heroPlayerID=-1;bool ownerIDRead=false,heroPlayerIDRead=false;
    int inventoryLayout=0;
    int shownBuffCount=0;char buffNames[16][128]={};int buffStacks[16]={};float buffDuration[16]={},buffExpires[16]={};
    bool visionRead=false;uint32_t visionMask=0;int team=0;
    bool npcVisionRead=false,npcVisible=false,sceneDormantRead=false,sceneDormant=false;
    int npcProbeMask=0;uint64_t npcWord=0,npcSelfWord=0;uint32_t npcDataHandle=0,npcIndex=0;
    int hp=0;bool alive=false,footProjected=false,headProjected=false,nodeRead=false,ownerMatches=false;
};
struct Snapshot {
    bool frameOk = false;
    bool observedOnly = false;
    bool matrixOk = false;
    bool projectedLocal = false;
    bool projectedHero = false;
    int unitCount = 0;
    int heroCount = 0;
    int projectedUnits = 0;
    float matrix[16] = {};
    Vec3 sampleWorld{};
    bool sampledWorld = false;
    int projectionMode = 0;
    int uprightVotes=0,flippedVotes=0,zeroOriginHeroes=0,samplesCount=0;
    HeroSample samples[12]{};
};
extern Snapshot snapshot;
void Update(const Frame& frame);
}
