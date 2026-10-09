#pragma once
#include "common.h"
#include "mem.h"
#include "offsets.h"
#include "buff_reader.h"
#include <mutex>
#include <vector>

enum class UnitKind : uint8_t {
    Unknown = 0,
    Hero,
    Creep,
    Ward,
    Roshan,
    Building,
    Courier,
    Boss,
    Rune,
    RuneSpawner,
};

inline const char* KindTag(UnitKind k) {
    switch (k) {
    case UnitKind::Hero:     return "HERO";
    case UnitKind::Creep:    return "CREEP";
    case UnitKind::Ward:     return "WARD";
    case UnitKind::Roshan:   return "ROSHAN";
    case UnitKind::Building: return "BUILDING";
    case UnitKind::Courier:  return "COURIER";
    case UnitKind::Boss:     return "BOSS";
    default:                 return "";
    }
}

struct StaticUnit {
    uint32_t     entityHandle = 0; // Snapshot generation; zero only for uncached/manual units.
    uintptr_t    addr = 0;
    UnitKind     kind = UnitKind::Unknown;
    const char*  cls  = nullptr;
    char         name[64] = {};
    char         nick[32] = {};
};

struct ItemInfo {
    int slot = -1, charges = -1;
    uintptr_t addr = 0;
    uint32_t instanceHandle = 0;
    float cd = -1.f, cdLen = 0.f, expiresAt = -1.f;
    bool cooldownRead = false;
    char icon[80] = {};
};

struct AbilityInfo {
    uintptr_t addr=0;uint32_t entityHandle=0;bool phaseRead=false;
    char        icon[80] = {};
    int         slot = -1;
    int         maxLevel = 0;
    bool        automationReady = false;
    bool        cooldownRead = false;
    int         level   = 0;
    int         mana    = 0;
    float       cd      = 0.f;
    float       cdLen   = 0.f;
    bool        phase   = false;
    const char* cls     = nullptr;
};

struct FrameUnit {
    uintptr_t   addr = 0;
    UnitKind    kind = UnitKind::Unknown;
    const char* cls  = nullptr;
    char        name[64] = {};
    char        nick[32] = {};

    int   team = 0;
    int   hp = 0, maxHp = 0;
    float mana = 0.f, maxMana = 0.f;
    bool manaRead=false;
    Vec3  pos{};
    float hbOffset = 200.f;

    bool  alive = false;
    bool  illusion = false;
    bool illusionRead=false,invisRead=false,stateRead=false,clockRead=false;
    uint64_t unitState=0;
    float sampleTime=0;
    float invis = 0.f;
    int   level = 0;
    float armor = 0.f;
    float dmgAvg = 0.f;
    int   atkRange = 0;

    float dist = 0.f;
    int   respawn = -1;

    uint32_t entityHandle = 0;
    int runeType = -1, nextRuneType = -1;
    float runeTime = -1.f, runeLastSpawn = -1.f, runeNextSpawn = -1.f;
    bool runeRead = false;

    bool buffsRead=false,buffsVisualRead=false;
    std::vector<buffreader::Buff> buffs;

    int playerId = -1;
    bool inventoryRead = false;
    uint64_t inventoryRaw0=0,inventoryRaw8=0;uint32_t inventoryParent=0;int inventoryProbeMask=0;
    int inventoryLayout=0;
    int inventoryCount = -1, inventoryResolved = 0, inventoryUnmapped = 0;
    bool aegisVisible = false, aegisEstimated = false;
    float aegisSeconds = 0.f;
    uint32_t teamVisibilityMask = 0;
    bool teamVisibilityRead = false;
    bool npcVisibilityRead=false,npcVisible=false,sceneDormantRead=false,sceneDormant=false;
    int npcVisibilityProbeMask=0;uint64_t npcVisibilityWord=0,npcVisibilitySelfWord=0;uint32_t npcVisibilityDataHandle=0,npcVisibilityIndex=0;
    int bountyMin = -1, bountyMax = -1;
    ItemInfo items[27];
    int itemN = 0;
    AbilityInfo abil[16];
    int         abilN = 0;

    bool  mdirOk = false;
    Vec3  mdir{};

    bool  canLastHit = false;
    bool  canDeny = false;
    int   hits = 0;
};

struct Frame {
    bool  ok = false;
    bool  observedOnly = false; // World data only; NEVER enables automation/local-player features.
    float now = 0.f;

    int   localTeam = 2;
    Vec3  localPos{};
    int   hp = 0, maxHp = 0, mana = 0, maxMana = 0, level = 0;
    float dmgAvg = 0.f;
    int   atkRange = 0;
    int damageMin = 0, damageMax = 0, damageBonus = 0;
    bool damageRead = false;
    bool  localAlive = false;

    uintptr_t localHero = 0;
    uint32_t localHandle=0;
    uintptr_t queryUnit = 0;
    uintptr_t rules = 0;
    float     gameStart = -1.f;
    int       gameState = -1;
    int       roshanPhase = -1;
    float     roshanEnd = -1.f;

    bool  roshanAlive = false;
    int   roshanHp = 0, roshanMaxHp = 0;
    Vec3  roshanPos{};
    bool  roshanPosValid = false;
    float roshanEtaLo = -1.f;
    float roshanEtaHi = -1.f;

    bool  lastHitDeny = true;

    std::vector<FrameUnit> units;
};

namespace game {

struct Sys {
    uintptr_t clientBase  = 0;
    uint32_t  imageSize   = 0;
    uintptr_t esys        = 0;
    uintptr_t idPages[off::maxIdPages] = {};
    uint32_t  handleMask  = off::handleMask;
    uintptr_t localCtrl   = 0;
    bool      rttiOk      = false;
    bool      ready       = false;
};

extern Sys g_sys;

// Atomic read-only telemetry for the failing initialization path.
struct EntityProbe {
    std::atomic<int> stage{0};
    std::atomic<unsigned> attempts{0};
    std::atomic<uintptr_t> slot{0}, pointer{0}, vtable{0}, expectedVtable{0};
    std::atomic<uintptr_t> word08{0}, word10{0}, word18{0};
    std::atomic<int> pages{0}, hits{0};
    std::atomic<unsigned> scannedSlots{0}, matchedObjects{0};
    std::atomic<int> lastScanStage{0};
};
extern EntityProbe g_probe;

struct ControllerProbe {
    std::atomic<int> globalStage{0}, source{0}, candidates{0}, localFlags{0}, frameStage{0}, scannedEntities{0}, classifiedUnits{0}, classifiedHeroes{0}, controllerRefStage{0}, controllerRefHits{0};
    std::atomic<unsigned> controllerRefSlots{0};
    std::atomic<uintptr_t> globalSlot{0}, globalValue{0}, globalVtable{0};
    std::atomic<uint32_t> heroHandle{0};
    std::atomic<uintptr_t> heroAddress{0};
    std::atomic<unsigned> scanPasses{0}, scanFaults{0}, exceptionCode{0};
    std::atomic<ULONGLONG> scanHeartbeat{0};
};
extern ControllerProbe g_controllerProbe;


extern std::atomic<uintptr_t> g_playerResource;
extern std::atomic<uintptr_t> g_teamVisibilityData[2];
extern std::atomic<uint32_t> g_teamVisibilityDataHandle[2];
extern uintptr_t g_rules;
extern uintptr_t g_rulesProxy;
extern uintptr_t g_roshanSpawner;
extern uintptr_t g_dataEnt;
extern uintptr_t g_visEnt;
extern uintptr_t g_modeEnt;

bool  TryInit();
void  ScanLoop();
void  BuildFrame(Frame& out, uintptr_t localCtrl);
void  BuildObservedFrame(Frame& out);

uintptr_t EntityByIndex(int index);
uintptr_t EntityByHandle(uint32_t handle);
void      RefreshModeEntity();

extern Vec3  g_roshanLastPos;
extern bool  g_roshanSeen;

}
