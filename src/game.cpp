#include "ability_max_levels.h"
#include "game.h"
#include "npc_visibility.h"
#include "offsets.h"
#include "damage_estimate.h"
#include <unordered_map>
#include <cstring>
#include <algorithm>

namespace game {

Sys g_sys;
EntityProbe g_probe;
ControllerProbe g_controllerProbe;

std::atomic<uintptr_t> g_playerResource{0};
std::atomic<uintptr_t> g_teamVisibilityData[2]{{0},{0}};
std::atomic<uint32_t> g_teamVisibilityDataHandle[2]{{0},{0}};
uintptr_t g_rules         = 0;
uintptr_t g_rulesProxy    = 0;
uintptr_t g_roshanSpawner = 0;
uintptr_t g_dataEnt       = 0;
uintptr_t g_visEnt        = 0;
uintptr_t g_modeEnt       = 0;

Vec3  g_roshanLastPos{};
bool  g_roshanSeen = false;

static std::mutex          g_lock;
static std::vector<StaticUnit> g_units;
struct AegisCandidate { uintptr_t addr=0,vptr=0; };
static std::vector<AegisCandidate> g_aegisEntities,g_frameAegis;



static uintptr_t IdentityAddr(int index) {
    if (index < 0 || index >= off::maxIdPages * off::entsPerPage) return 0;
    uintptr_t page = g_sys.idPages[index >> 9];
    if (!page) return 0;
    return page + (uintptr_t)(index & (off::entsPerPage - 1)) * (uintptr_t)off::idStride;
}

static bool RefreshPages() {
    if (!g_sys.esys) return false;
    bool any = false;
    for (int k = 0; k < off::maxIdPages; ++k) {
        uintptr_t p = 0;
        mem::Read(g_sys.esys + off::idPagesOff + 8ULL * k, p);
        g_sys.idPages[k] = mem::ValidPtr(p) ? p : 0;
        if (g_sys.idPages[k]) any = true;
    }
    return any;
}

static uintptr_t ResolveIndexEx(int index, uint32_t exactHandle) {
    uintptr_t id = IdentityAddr(index);
    if (!id) return 0;
    uint32_t fld = 0;
    if (!mem::Read(id + off::idHandleFld, fld)) return 0;
    if ((fld & g_sys.handleMask) != (uint32_t)index) return 0;
    if (exactHandle && fld != exactHandle) return 0;
    uintptr_t e = 0;
    if (!mem::Read(id, e) || !mem::ValidPtr(e)) return 0;
    uintptr_t back = 0;
    if (!mem::Read(e + off::instEntity, back) || back != id) return 0;
    return e;
}

static uintptr_t ResolveIndex(int index) {
    return ResolveIndexEx(index, 0);
}

uintptr_t EntityByIndex(int index) {
    if (!g_sys.ready) return 0;
    return ResolveIndex(index);
}

uintptr_t EntityByHandle(uint32_t handle) {
    if (!g_sys.ready || handle == 0 || handle == 0xFFFFFFFFu) return 0;
    int idx = (int)(handle & g_sys.handleMask);
    // Fail closed on recycled slots: never resolve a stale serial to a new entity.
    return ResolveIndexEx(idx, handle);
}

void RefreshModeEntity() {
    if (!g_sys.ready) return;

    if (g_modeEnt) {
        uint8_t b = 0xFF;
        if (mem::Read(g_modeEnt + off::GameMode::m_bFogOfWarDisabled, b) && b <= 1) return;
        g_modeEnt = 0;
    }
    if (!g_rules) return;

    uint32_t h = 0;
    if (!mem::Read(g_rules + off::Rules::m_hGameModeEntity, h)) return;
    uintptr_t e = EntityByHandle(h);
    if (!e) return;
    uintptr_t vptr = 0;
    if (!mem::Read(e, vptr) || vptr < g_sys.clientBase ||
        vptr >= g_sys.clientBase + g_sys.imageSize) return;

    uint64_t lo = 0;
    uint32_t hi = 0;
    if (!mem::Read(e + 0x600, lo) || !mem::Read(e + 0x608, hi)) return;
    const unsigned char* b = (const unsigned char*)&lo;
    for (int i = 0; i < 8; ++i)
        if (b[i] > 1) return;
    const unsigned char* c = (const unsigned char*)&hi;
    for (int i = 0; i < 3; ++i)
        if (c[i] > 1) return;

    g_modeEnt = e;
}

static bool LooksLikeUnitName(const char* s) {
    if (!s || !*s) return false;
    if (!StartsWith(s, "npc_")) return false;
    for (const char* c = s; *c; ++c) {
        char ch = *c;
        bool ok = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (!ok) return false;
    }
    return true;
}

static bool ReadUnitName(uintptr_t e, char* out, int cap) {
    uintptr_t p = 0;
    if (!mem::Read(e + off::NPC::m_iszUnitName, p)) return false;
    if (p < 0x100000000ULL || p > 0x00007FFFFFFFFFFFULL) return false;
    char buf[64] = {};
    if (!mem::ReadStr(p, buf, sizeof(buf))) return false;
    if (!LooksLikeUnitName(buf)) return false;
    strncpy(out, buf, cap - 1);
    out[cap - 1] = 0;
    return true;
}

static bool ReadDesignerUnitName(uintptr_t e,char* out,int cap) {
    uintptr_t identity=0,back=0,name=0;uint32_t handle=0;
    if(!mem::Read(e+off::instEntity,identity)||!mem::ValidPtr(identity)||
       !mem::Read(identity,back)||back!=e||!mem::Read(identity+off::idHandleFld,handle)||
       !handle||handle==0xFFFFFFFFu||ResolveIndexEx((int)(handle&g_sys.handleMask),handle)!=e||
       !mem::Read(identity+off::Identity::m_designerName,name)||name<0x100000000ULL||name>0x00007FFFFFFFFFFFULL)return false;
    char text[64]={};if(!mem::ReadStr(name,text,sizeof(text))||!LooksLikeUnitName(text))return false;
    strncpy(out,text,cap-1);out[cap-1]=0;return true;
}

static void MakeNick(const char* cls, const char* name, char* out, int cap) {
    out[0] = 0;
    static const char* kHeroCls = "C_DOTA_Unit_Hero_";
    if (cls && StartsWith(cls, kHeroCls)) {
        strncpy(out, cls + strlen(kHeroCls), cap - 1);
        out[cap - 1] = 0;
        return;
    }
    static const char* kHeroName = "npc_dota_hero_";
    if (StartsWith(name, kHeroName)) {
        strncpy(out, name + strlen(kHeroName), cap - 1);
        out[cap - 1] = 0;
        return;
    }
}

static bool RulesPlausible(uintptr_t p) {
    if (!mem::ValidPtr(p)) return false;
    int state = mem::ReadOr<int>(p + off::Rules::m_nGameState, -1);
    if (state < 0 || state > 64) return false;
    float t = mem::ReadOr<float>(p + off::Rules::m_flGameStartTime, -1.f);
    if (!(t >= 0.f && t < 300000.f)) return false;
    int gold = mem::ReadOr<int>(p + off::Rules::m_nStartingGold, -1);
    if (gold < 0 || gold > 10000) return false;
    int mode = mem::ReadOr<int>(p + off::Rules::m_iGameMode, -1);
    if (mode < 0 || mode > 1000) return false;
    uint32_t phase = mem::ReadOr<uint32_t>(p + off::Rules::m_nRoshanRespawnPhase, 99);
    return phase <= 2;
}

struct CacheEntry {
    uintptr_t identity = 0;
    uint32_t handle = 0;
    uintptr_t vptr = 0;
    UnitKind  kind = UnitKind::Unknown;
    const char* cls = nullptr;
    char      name[64] = {};
    char      nick[32] = {};
};

static std::unordered_map<uintptr_t, CacheEntry> g_cache;

struct MoveHist { Vec3 pos{}; float sim = -1.f; Vec3 dir{}; bool ok = false; };
static std::unordered_map<uintptr_t, MoveHist> g_moveHist;
static std::vector<StaticUnit> g_frameSnapshot;

static bool IsAnnouncer(const char* cls,const char* name) {
    // Announcer units can use npc_dota_hero_* identifiers, but are not playable heroes.
    return (cls&&(Contains(cls,"Announcer")||Contains(cls,"announcer"))) ||
        (name&&StartsWith(name,"npc_dota_hero_announcer"));
}

static bool Classify(uintptr_t e, uintptr_t vptr, StaticUnit& u) {
    const char* cls = rtti::ClassOf(g_sys.clientBase, vptr);

    if (cls) {
        if (Streq(cls, "C_DOTAGamerulesProxy")) {
            uintptr_t p = 0;
            mem::Read(e + off::RulesProxy::m_pGameRules, p);
            if (RulesPlausible(p)) { g_rules = p; g_rulesProxy = e; }
            return false;
        }
        if (Streq(cls, "C_DOTA_RoshanSpawner")) { g_roshanSpawner = e; return false; }
        if (StartsWith(cls, "C_DOTA_Data") &&
            !Streq(cls, "C_DOTA_DataSpectator")) { g_dataEnt = e; return false; }
        if (Contains(cls, "PlayerVisibility")) { g_visEnt = e; return false; }
    }

    char name[64] = {};
    bool runeClass=cls && (Streq(cls,"C_DOTA_Item_Rune") || StartsWith(cls,"C_DOTA_Item_RuneSpawner"));
    bool hasName = !runeClass && (ReadUnitName(e,name,sizeof(name)) || ReadDesignerUnitName(e,name,sizeof(name))); // Avoid NPC-only fields on spawners.

    if(IsAnnouncer(cls,hasName?name:nullptr))return false;
    UnitKind kind = UnitKind::Unknown;

    if (cls && (Streq(cls,"C_DOTA_Item_RuneSpawner") ||
                Streq(cls,"C_DOTA_Item_RuneSpawner_Bounty") ||
                Streq(cls,"C_DOTA_Item_RuneSpawner_Powerup") ||
                Streq(cls,"C_DOTA_Item_RuneSpawner_XP"))) kind = UnitKind::RuneSpawner;
    else if (Streq(cls,"C_DOTA_Item_Rune")) kind = UnitKind::Rune;
    else if (cls && StartsWith(cls, "C_DOTA_Unit_Hero_")) kind = UnitKind::Hero;
    else if (hasName && StartsWith(name, "npc_dota_hero_")) kind = UnitKind::Hero;

    if (kind == UnitKind::Unknown && hasName) {
        if (Streq(name, "npc_dota_roshan")) kind = UnitKind::Roshan;
        else if (Contains(name, "observer_ward") || Contains(name, "sentry_ward") ||
                 StartsWith(name, "npc_dota_ward")) kind = UnitKind::Ward;
        else if (StartsWith(name, "npc_dota_creep") || StartsWith(name, "npc_dota_neutral") ||
                 StartsWith(name, "npc_dota_siege")) kind = UnitKind::Creep;
        else if (Contains(name, "_tower") || Contains(name, "_fort") ||
                 Contains(name, "_fountain") || Contains(name, "_healer")) kind = UnitKind::Building;
        else if (Contains(name, "courier")) kind = UnitKind::Courier;
        else if (Contains(name, "tormentor") || Contains(name, "miniboss")) kind = UnitKind::Boss;
    }

    if (kind == UnitKind::Unknown) {
        if (cls && (Contains(cls, "_Creep") || StartsWith(cls, "C_DOTA_Unit_Creep"))) kind = UnitKind::Creep;
        else if (cls && Contains(cls, "Building") && StartsWith(cls, "C_DOTA_")) kind = UnitKind::Building;
    }

    if (kind == UnitKind::Unknown) return false;

    u.addr = e;
    u.kind = kind;
    u.cls  = cls;
    strncpy(u.name, name, sizeof(u.name) - 1);
    MakeNick(cls, name, u.nick, sizeof(u.nick));
    return true;
}

// A pointer + vtable is not an entity identity: the engine can recycle both.
// Never cache a negative result; names/RTTI can become available a later pass.
static bool CachedClassify(uintptr_t e, uintptr_t vptr, StaticUnit& u) {
    uintptr_t identity = 0;
    uint32_t handle = 0;
    if (!mem::Read(e + off::instEntity, identity) || !mem::ValidPtr(identity) ||
        !mem::Read(identity + off::idHandleFld, handle) || !handle || handle == 0xFFFFFFFFu ||
        ResolveIndexEx((int)(handle & g_sys.handleMask), handle) != e) {
        g_cache.erase(e);
        return false;
    }
    auto it = g_cache.find(e);
    if (it != g_cache.end() && it->second.vptr == vptr &&
        it->second.identity == identity && it->second.handle == handle &&
        it->second.cls == rtti::ClassOf(g_sys.clientBase, vptr)) {
        const auto& ce = it->second;
        u = StaticUnit{};
        u.entityHandle = handle; u.addr = e; u.kind = ce.kind; u.cls = ce.cls;
        memcpy(u.name, ce.name, sizeof(u.name));
        memcpy(u.nick, ce.nick, sizeof(u.nick));
        return true;
    }
    g_cache.erase(e);
    u = StaticUnit{};
    if (!Classify(e, vptr, u)) return false;
    u.entityHandle = handle;
    CacheEntry ce;
    ce.identity = identity; ce.handle = handle; ce.vptr = vptr;
    ce.kind = u.kind; ce.cls = u.cls;
    memcpy(ce.name, u.name, sizeof(ce.name));
    memcpy(ce.nick, u.nick, sizeof(ce.nick));
    if (g_cache.size() > 24576) g_cache.clear();
    g_cache[e] = ce;
    return true;
}

static bool IsPlayerControllerVtable(uintptr_t vp) {
    if (vp < g_sys.clientBase || vp >= g_sys.clientBase + g_sys.imageSize) return false;
    // Keep the known exact address, with a named RTTI fallback from the same dump.
    const char* cls=rtti::ClassOf(g_sys.clientBase,vp);
    if(cls)return Streq(cls,"C_DOTAPlayerController"); // A known wrong class overrides an old RVA.
    return vp==g_sys.clientBase+off::Ctrl::vtableRva;
}

// All checks in this resolver are reads; no layout checks are bypassed.
static bool ValidateEntityCandidate(uintptr_t object, uintptr_t slot) {
    if (!mem::ValidPtr(object)) return false;
    uintptr_t vp = 0;
    if (!mem::Read(object, vp) || vp != g_sys.clientBase + off::esysVtableRva)
        return false;
    g_probe.matchedObjects.fetch_add(1);
    g_probe.slot.store(slot);
    g_probe.pointer.store(object);
    g_probe.vtable.store(vp);
    uintptr_t word = 0;
    if (mem::Read(object + 0x08, word)) g_probe.word08.store(word);
    word = 0;
    if (mem::Read(object + 0x10, word)) g_probe.word10.store(word);
    word = 0;
    if (mem::Read(object + 0x18, word)) g_probe.word18.store(word);
    g_sys.esys = object;
    if (!RefreshPages()) {
        g_probe.stage.store(6);
        g_sys.esys = 0;
        return false;
    }
    int pages = 0;
    for (int i = 0; i < off::maxIdPages; ++i)
        if (g_sys.idPages[i]) ++pages;
    g_probe.pages.store(pages);
    int hits = 0;
    // Check all configured pages, not just indices 1..512.
    const int maxIndex = off::maxIdPages * off::entsPerPage;
    for (int i = 1; i < maxIndex && hits < 8; ++i)
        if (ResolveIndex(i)) ++hits;
    g_probe.hits.store(hits);
    if (hits < 2) {
        g_probe.stage.store(7);
        g_sys.esys = 0;
        return false;
    }
    g_probe.stage.store(8);
    return true;
}

static bool SearchEntitySystemData() {
    uintptr_t base = g_sys.clientBase;
    IMAGE_DOS_HEADER dos{};
    if (!mem::Read(base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || (uint32_t)dos.e_lfanew > g_sys.imageSize)
        return false;
    IMAGE_NT_HEADERS64 nt{};
    if (!mem::Read(base + (uintptr_t)dos.e_lfanew, nt) ||
        nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.FileHeader.NumberOfSections > 96)
        return false;
    const uintptr_t sectionTable = base + (uintptr_t)dos.e_lfanew +
        sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;
    const uintptr_t expected = base + off::esysVtableRva;
    const unsigned maxSlots = 2U * 1024U * 1024U; // 16 MiB maximum per pass.
    unsigned scanned = 0;
    g_probe.stage.store(9);
    for (unsigned n = 0; n < nt.FileHeader.NumberOfSections; ++n) {
        if (!cfg::running.load()) return false;
        IMAGE_SECTION_HEADER sh{};
        if (!mem::Read(sectionTable + n * sizeof(sh), sh)) continue;
        if (!(sh.Characteristics & IMAGE_SCN_MEM_READ) ||
            !(sh.Characteristics & IMAGE_SCN_MEM_WRITE) ||
            (sh.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        uint32_t rva = sh.VirtualAddress;
        if (rva >= g_sys.imageSize) continue;
        uint32_t size = sh.Misc.VirtualSize;
        if (size > g_sys.imageSize - rva) size = g_sys.imageSize - rva;
        for (uint32_t offset = 0; offset + sizeof(uintptr_t) <= size; offset += sizeof(uintptr_t)) {
            if (!cfg::running.load()) return false;
            if (scanned >= maxSlots) {
                g_probe.scannedSlots.store(scanned);
                g_probe.stage.store(11);
                return false;
            }
            if ((scanned & 8191U) == 0) {
                g_probe.scannedSlots.store(scanned);
                Sleep(1); // Yield on the initialization thread, not rendering thread.
            }
            ++scanned;
            uintptr_t slot = base + rva + offset;
            uintptr_t value = 0;
            if (!mem::Read(slot, value)) continue;
            // Handle a direct static object and a global pointer to an object.
            if (value == expected) {
                if (ValidateEntityCandidate(slot, slot)) {
                    g_probe.scannedSlots.store(scanned);
                    return true;
                }
            } else if (mem::ValidPtr(value)) {
                uintptr_t first = 0;
                if (mem::Read(value, first) && first == expected) {
                    if (ValidateEntityCandidate(value, slot)) {
                        g_probe.scannedSlots.store(scanned);
                        return true;
                    }
                }
            }
        }
    }
    g_probe.scannedSlots.store(scanned);
    if (!g_probe.matchedObjects.load()) g_probe.stage.store(10);
    // If objects matched but validation failed, retain stage 6 or 7.
    return false;
}

static bool DiscoverEntitySystem() {
    static ULONGLONG nextScanAt = 0;
    if (GetTickCount64() < nextScanAt) {
        g_probe.stage.store(12);
        return false;
    }
    g_probe.matchedObjects.store(0);
    uintptr_t base = g_sys.clientBase;
    g_sys.esys = 0;
    g_probe.attempts.fetch_add(1);
    g_probe.stage.store(1);
    g_probe.pointer.store(0);
    g_probe.vtable.store(0);
    g_probe.expectedVtable.store(base + off::esysVtableRva);
    g_probe.word08.store(0);
    g_probe.word10.store(0);
    g_probe.word18.store(0);
    g_probe.pages.store(0);
    g_probe.hits.store(0);

    for (size_t i = 0; i < sizeof(off::esysRva) / sizeof(off::esysRva[0]); ++i) {
        uintptr_t slot = base + off::esysRva[i];
        g_probe.slot.store(slot);
        uintptr_t value = 0;
        if (!mem::Read(slot, value)) { g_probe.stage.store(2); continue; }
        g_probe.pointer.store(value);
        if (ValidateEntityCandidate(value, slot)) {g_probe.lastScanStage.store(8);return true;}
        if (g_probe.matchedObjects.load()) continue;
        if (!mem::ValidPtr(value)) { g_probe.stage.store(3); continue; }
        uintptr_t first = 0;
        if (!mem::Read(value, first)) { g_probe.stage.store(4); continue; }
        g_probe.vtable.store(first);
        if (first != base + off::esysVtableRva) g_probe.stage.store(5);
    }

    g_probe.scannedSlots.store(0);
    g_probe.matchedObjects.store(0);
    bool found = SearchEntitySystemData();
    g_probe.lastScanStage.store(g_probe.stage.load());
    nextScanAt = GetTickCount64() + 30000;
    return found;
}

static bool DetectMask(uintptr_t ctrl) {
    uint32_t h = 0;
    if (!mem::Read(ctrl + off::Ctrl::m_hAssignedHero, h)) return false;
    if (h == 0 || h == 0xFFFFFFFFu) return false;

    static const uint32_t kMasks[] = { 0x3FFF, 0x7FFF, 0x1FFF, 0xFFFF, 0xFFF };
    for (uint32_t m : kMasks) {
        int idx = (int)(h & m);
        uintptr_t id = IdentityAddr(idx);
        if (!id) continue;
        uint32_t fld = 0;
        if (!mem::Read(id + off::idHandleFld, fld) || fld != h) continue;
        uintptr_t e = 0;
        if (!mem::Read(id, e) || !mem::ValidPtr(e)) continue;
        uintptr_t back = 0;
        if (!mem::Read(e + off::instEntity, back) || back != id) continue;

        char nm[64] = {};
        if (ReadUnitName(e, nm, sizeof(nm)) && StartsWith(nm, "npc_dota_hero_") && !IsAnnouncer(nullptr,nm)) {
            g_sys.handleMask = m;
            return true;
        }
    }
    return false;
}

static bool IsHeroEntity(uintptr_t hero) {
    uintptr_t vp = 0;
    if (!mem::ValidPtr(hero) || !mem::Read(hero, vp) ||
        vp < g_sys.clientBase || vp >= g_sys.clientBase + g_sys.imageSize) return false;
    const char* cls = rtti::ClassOf(g_sys.clientBase, vp);
    char name[64] = {};
    bool hasName=ReadUnitName(hero,name,sizeof(name));
    if(IsAnnouncer(cls,hasName?name:nullptr))return false;
    if(cls&&StartsWith(cls,"C_DOTA_Unit_Hero_"))return true;
    return hasName&&StartsWith(name,"npc_dota_hero_");
}

// Dedicated hero player ID can prove demo ownership when generic NPC owner is -1.
// Any valid conflicting ID is rejected, not overridden by another field.
static bool HeroBelongsToPlayer(uintptr_t hero,int player){
 if(player<0||player>=64||!IsHeroEntity(hero))return false;
 int owner=-1,heroPlayer=-1;bool ownerRead=mem::Read(hero+off::NPC::m_nPlayerOwnerID,owner);
 bool heroRead=mem::Read(hero+off::Hero::m_iPlayerID,heroPlayer);
 if((ownerRead&&(owner<-1||owner>=64))||(heroRead&&(heroPlayer<-1||heroPlayer>=64)))return false;
 bool ownerValid=ownerRead&&owner>=0&&owner<64,heroValid=heroRead&&heroPlayer>=0&&heroPlayer<64;
 if((ownerValid&&owner!=player)||(heroValid&&heroPlayer!=player))return false;
 return ownerValid||heroValid;
}
// A scanned controller must carry the local flag. The dump's dedicated local
// global may also be used with flag=0, but only with a verified owned hero.
static bool ControllerCandidate(uintptr_t ctrl, bool fromGlobal) {
    uintptr_t vp = 0;
    if (!mem::ValidPtr(ctrl) || !mem::Read(ctrl, vp) || !IsPlayerControllerVtable(vp)) return false;
    uint8_t local = 2;
    if (!mem::Read(ctrl + off::Ctrl::m_bIsLocalPlayerController, local) || local > 1) return false;
    if (local == 1) return true;
    if (!fromGlobal) return false;
    int player = -1;
    uint32_t h = 0;
    if (!mem::Read(ctrl + off::Ctrl::m_nPlayerID, player) || player < 0 || player >= 64 ||
        !mem::Read(ctrl + off::Ctrl::m_hAssignedHero, h) || !h || h == 0xFFFFFFFFu) return false;
    uintptr_t hero = EntityByHandle(h);
    if (!hero && DetectMask(ctrl)) hero = EntityByHandle(h);
    return HeroBelongsToPlayer(hero,player);
}

static uintptr_t GlobalLocalController() {
    // CTRL1 live screenshot proved this dump's named global is C_DOTAGamerules.
    // Do not continue probing/using it as a player pointer.
    g_controllerProbe.globalSlot.store(0);
    g_controllerProbe.globalValue.store(0);
    g_controllerProbe.globalVtable.store(0);
    g_controllerProbe.globalStage.store(7);
    return 0;
}

static uintptr_t ChooseLocalController(uintptr_t globalCtrl, uintptr_t scannedCtrl, int localFlags) {
    if (globalCtrl) {g_controllerProbe.source.store(1);return globalCtrl;}
    if (localFlags == 1) {g_controllerProbe.source.store(2);return scannedCtrl;}
    g_controllerProbe.source.store(localFlags > 1 ? 3 : 0);return 0;
}

struct ControllerReferenceScan {
    uintptr_t base=0,cached=0;std::vector<std::pair<uintptr_t,uintptr_t>> ranges;
    size_t range=0;uintptr_t cursor=0;uint64_t retryAt=0;bool collecting=false,ambiguous=false,truncated=false;
    uintptr_t found=0;unsigned slots=0;
};
static ControllerReferenceScan g_controllerRefs;
static bool StrictLocalReference(uintptr_t ctrl) {
    uintptr_t vp=0;uint32_t h=0;int player=-1;uint8_t local=2;
    if(!mem::ValidPtr(ctrl)||!mem::Read(ctrl,vp)||!IsPlayerControllerVtable(vp)||
       !mem::Read(ctrl+off::Ctrl::m_bIsLocalPlayerController,local)||local!=1||
       !mem::Read(ctrl+off::Ctrl::m_nPlayerID,player)||player<0||player>=64||
       !mem::Read(ctrl+off::Ctrl::m_hAssignedHero,h)||!h||h==0xFFFFFFFFu)return false;
    uintptr_t hero=EntityByHandle(h);
    if(!HeroBelongsToPlayer(hero,player))return false;
    uint8_t localAgain=2;uint32_t handleAgain=0;
    return mem::Read(ctrl+off::Ctrl::m_bIsLocalPlayerController,localAgain)&&localAgain==1&&
           mem::Read(ctrl+off::Ctrl::m_hAssignedHero,handleAgain)&&handleAgain==h;
}
static uintptr_t SearchLocalControllerRefs(unsigned budget=16384) {
    auto& s=g_controllerRefs;auto& p=g_controllerProbe;
    if(s.base!=g_sys.clientBase){s=ControllerReferenceScan{};s.base=g_sys.clientBase;}
    if(s.cached){if(StrictLocalReference(s.cached)){p.controllerRefStage.store(4);return s.cached;}s.cached=0;s.retryAt=0;}
    uint64_t now=GetTickCount64();if(!s.collecting&&now<s.retryAt)return 0;
    if(!s.collecting) {
        s.ranges.clear();s.range=0;s.found=0;s.ambiguous=false;s.truncated=false;s.slots=0;p.controllerRefHits.store(0);
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(!mem::Read(s.base,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<=0||
           (uint32_t)dos.e_lfanew>g_sys.imageSize||!mem::Read(s.base+dos.e_lfanew,nt)||
           nt.Signature!=IMAGE_NT_SIGNATURE||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt.FileHeader.NumberOfSections>96){p.controllerRefStage.store(1);s.retryAt=now+30000;return 0;}
        uintptr_t table=s.base+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
        uint64_t bytes=0;
        for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i){IMAGE_SECTION_HEADER sh{};
            if(!mem::Read(table+i*sizeof(sh),sh)||!(sh.Characteristics&IMAGE_SCN_MEM_READ)||!(sh.Characteristics&IMAGE_SCN_MEM_WRITE)||(sh.Characteristics&IMAGE_SCN_MEM_EXECUTE)||sh.VirtualAddress>=g_sys.imageSize)continue;
            uint32_t size=(uint32_t)sh.Misc.VirtualSize;if(size>g_sys.imageSize-sh.VirtualAddress)size=g_sys.imageSize-sh.VirtualAddress;
            if(uint64_t(size)>uint64_t(16*1024*1024)-bytes)s.truncated=true;
            size=(uint32_t)std::min(uint64_t(size),uint64_t(16*1024*1024)-bytes);size&=~uint32_t(7);
            if(size){s.ranges.emplace_back(s.base+sh.VirtualAddress,s.base+sh.VirtualAddress+size);bytes+=size;}
        }
        if(s.ranges.empty()){p.controllerRefStage.store(1);s.retryAt=now+30000;return 0;}
        s.cursor=s.ranges[0].first;s.collecting=true;
    }
    p.controllerRefStage.store(2);
    for(unsigned n=0;n<budget&&s.range<s.ranges.size();++n){
        if(!cfg::running.load())return 0;
        uintptr_t slot=s.cursor,value=0,ctrl=0;++s.slots;s.cursor+=8;
        if(mem::Read(slot,value)){
            if(IsPlayerControllerVtable(value))ctrl=slot;
            else if(mem::ValidPtr(value)){uintptr_t vp=0;if(mem::Read(value,vp)&&IsPlayerControllerVtable(vp))ctrl=value;}
            if(ctrl&&StrictLocalReference(ctrl)){if(!s.found){s.found=ctrl;p.controllerRefHits.store(1);}else if(s.found!=ctrl){s.ambiguous=true;p.controllerRefHits.store(2);}}
        }
        if(s.cursor>=s.ranges[s.range].second){++s.range;if(s.range<s.ranges.size())s.cursor=s.ranges[s.range].first;}
    }
    p.controllerRefSlots.store(s.slots);
    if(s.range<s.ranges.size())return 0; // Never choose the first hit before checking ambiguity.
    s.collecting=false;s.retryAt=now+30000;
    if(s.truncated){p.controllerRefStage.store(6);return 0;}
    if(!s.ambiguous&&s.found&&StrictLocalReference(s.found)){s.cached=s.found;p.controllerRefStage.store(4);return s.cached;}
    p.controllerRefStage.store(s.ambiguous?5:3);return 0;
}

static DWORD g_nextProbeAt = 0;
static std::atomic<bool> g_initBusy{ false };

static bool TryInitInner() {
    if (g_sys.ready) return true;

    uintptr_t base = mem::ModuleBase("client.dll");
    if (!base) return false;
    if (!g_sys.clientBase) {
        g_sys.clientBase = base;
        g_sys.imageSize = mem::ModuleSize(base);
    }

    if (GetTickCount() < g_nextProbeAt) return false;

    if (!DiscoverEntitySystem()) {
        g_nextProbeAt = GetTickCount() + 3000;
        return false;
    }

    g_sys.ready = true;
    return true;
}

bool TryInit() {
    if (g_sys.ready) return true;

    if (g_initBusy.exchange(true)) return false;
    bool ok = TryInitInner();
    g_initBusy.store(false);
    return ok;
}

void ScanLoop() {
    while (cfg::running.load()) {
        g_controllerProbe.scanHeartbeat.store(GetTickCount64());
        g_controllerProbe.scanPasses.fetch_add(1);
        if (!g_sys.ready) { Sleep(250); continue; }

        RefreshPages();
        RefreshModeEntity();

        std::vector<StaticUnit> next;
        next.reserve(384);

        uintptr_t sample[128];
        int sampleN = 0;
        int found = 0;
        uintptr_t localCtrl = 0;
        int controllerCount = 0, localFlagCount = 0;
        uintptr_t playerResource=0;int resourceCount=0;
        uintptr_t teamData[2]{};uint32_t teamDataHandles[2]{};int teamDataCount[2]{};
        uintptr_t globalCtrl = GlobalLocalController();

        std::vector<AegisCandidate> nextAegis;
        const int kMaxIndex = off::maxIdPages * off::entsPerPage;
        for (int i = 0; i < kMaxIndex; ++i) {
            uintptr_t e = ResolveIndex(i);
            if (!mem::ValidPtr(e)) continue;

            uintptr_t vptr = 0;
            if (!mem::Read(e, vptr)) continue;
            if (vptr < g_sys.clientBase || vptr >= g_sys.clientBase + g_sys.imageSize) continue;

            ++found;
            if (sampleN < 128) sample[sampleN++] = vptr;

            if (IsPlayerControllerVtable(vptr)) {
                ++controllerCount;
                if (ControllerCandidate(e, false)) {
                    ++localFlagCount;
                    localCtrl = e;
                }
            }

            // Collect only an exact Aegis RTTI match while doing the existing
            // entity scan. This does not guess an inventory-vector layout.
            const char* itemClass=rtti::ClassOf(g_sys.clientBase,vptr);
            if(Streq(itemClass,"C_DOTA_PlayerResource")){playerResource=e;++resourceCount;}
            if(Streq(itemClass,"C_DOTA_DataRadiant")){teamData[0]=e;mem::Read(IdentityAddr(i)+off::idHandleFld,teamDataHandles[0]);++teamDataCount[0];}
            if(Streq(itemClass,"C_DOTA_DataDire")){teamData[1]=e;mem::Read(IdentityAddr(i)+off::idHandleFld,teamDataHandles[1]);++teamDataCount[1];}
            if(Streq(itemClass,"C_DOTA_Item_Aegis")&&nextAegis.size()<32)nextAegis.push_back({e,vptr});
            StaticUnit u;
            if (CachedClassify(e, vptr, u)) next.push_back(u);
        }

        g_playerResource.store(resourceCount==1?playerResource:0);
        for(int team=0;team<2;++team){g_teamVisibilityDataHandle[team].store(teamDataCount[team]==1?teamDataHandles[team]:0);g_teamVisibilityData[team].store(teamDataCount[team]==1?teamData[team]:0);}
        if (!rtti::Ready() && sampleN >= 8)
            rtti::DetectDelta(g_sys.clientBase, sample, sampleN);
        g_sys.rttiOk = rtti::Ready();

        if (!g_rules) {
            for (int i = 1; i < 0x400 && !g_rules; ++i) {
                uintptr_t e = ResolveIndex(i);
                if (!mem::ValidPtr(e)) continue;
                uintptr_t p = 0;
                mem::Read(e + off::RulesProxy::m_pGameRules, p);
                if (RulesPlausible(p)) {
                    g_rules = p;
                    g_rulesProxy = e;
                }
            }
        }

        g_controllerProbe.classifiedUnits.store((int)next.size());
        int classifiedHeroes=0;for(const auto& u:next)if(u.kind==UnitKind::Hero)++classifiedHeroes;
        g_controllerProbe.classifiedHeroes.store(classifiedHeroes);
        g_controllerProbe.scannedEntities.store(found);
        g_controllerProbe.candidates.store(controllerCount);
        g_controllerProbe.localFlags.store(localFlagCount);
        // Never arbitrarily choose one of several local-marked controllers.
        localCtrl = ChooseLocalController(globalCtrl, localCtrl, localFlagCount);
        if(!localCtrl && localFlagCount==0){localCtrl=SearchLocalControllerRefs();if(localCtrl)g_controllerProbe.source.store(4);}
        if (localCtrl != g_sys.localCtrl) g_sys.localCtrl = localCtrl;
        if (g_sys.localCtrl) DetectMask(g_sys.localCtrl);
        g_controllerProbe.scanHeartbeat.store(GetTickCount64());

        {
            static int s_emptyPass = 0;
            if (found == 0) {
                if (++s_emptyPass >= 15 && g_sys.ready) {
                    s_emptyPass = 0;
                    g_sys.ready = false;
                    g_nextProbeAt = 0;
                }
            } else {
                s_emptyPass = 0;
            }
        }

        {
            std::lock_guard<std::mutex> lk(g_lock);
            g_units.swap(next);
            g_aegisEntities.swap(nextAegis);
        }

        Sleep(200);
    }
}

static void CopyUnits(std::vector<StaticUnit>& out) {
    std::lock_guard<std::mutex> lk(g_lock);
    out = g_units;
    g_frameAegis = g_aegisEntities;
}

struct IconMapping { const char* cls; const char* icon; bool item; };
#include "hud_icon_map.inc"

static const char* MappedIcon(const char* cls, bool item) {
    if (!cls) return nullptr;
    int lo = 0, hi = (int)(sizeof(kIconMappings) / sizeof(kIconMappings[0])) - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(cls, kIconMappings[mid].cls);
        if (!cmp) return kIconMappings[mid].item == item ? kIconMappings[mid].icon : nullptr;
        if (cmp < 0) hi = mid - 1; else lo = mid + 1;
    }
    return nullptr;
}

// A bounded fallback for generic/unmapped item RTTI. Entity identity names are
// schema-backed, not scanned offsets; only safe item_ ASCII identifiers are accepted.
static bool ItemIdentityName(uintptr_t item, char* icon, int cap) {
    uintptr_t identity=0;
    if(!mem::Read(item+off::EntityInstance::m_pEntity,identity)||!mem::ValidPtr(identity))return false;
    const uintptr_t fields[]={off::Identity::m_name,off::Identity::m_designerName};
    for(uintptr_t field:fields) {
        uintptr_t str=0;char name[96]={};
        if(!mem::Read(identity+field,str)||!mem::ReadStr(str,name,sizeof(name))||!StartsWith(name,"item_"))continue;
        const char* suffix=name+5;int n=(int)strlen(suffix);if(n<1||n>=cap)continue;
        bool valid=true;for(const char* c=suffix;*c;++c)
            if(!((*c>='a'&&*c<='z')||(*c>='0'&&*c<='9')||*c=='_'))valid=false;
        if(valid){strncpy(icon,suffix,cap-1);icon[cap-1]=0;return true;}
    }
    return false;
}
static void ReadItems(uintptr_t npc, FrameUnit& unit) {
    unit.itemN=0;unit.inventoryRead=false;unit.inventoryCount=-1;unit.inventoryLayout=0;
    unit.inventoryResolved=unit.inventoryUnmapped=0;
    uintptr_t inventory=npc+off::NPC::m_Inventory,vector=inventory+off::Inventory::m_hItems;
    unit.inventoryProbeMask=0;
    if(mem::Read(vector,unit.inventoryRaw0))unit.inventoryProbeMask|=1;
    if(mem::Read(vector+8,unit.inventoryRaw8))unit.inventoryProbeMask|=2;
    if(mem::Read(inventory+off::Inventory::m_hInventoryParent,unit.inventoryParent))unit.inventoryProbeMask|=4;
    struct Shape {int count,data,code;};const Shape shapes[]={{0,8,1},{8,0,2},{0,4,3}};
    int matches=0,parity=0;FrameUnit selected;
    if(!mem::Read(inventory+off::Inventory::m_iParity,parity))return;
    for(auto shape:shapes){
    int count=0;uintptr_t data=0;
    if(!mem::Read(vector+shape.count,count)||count<0||count>27)continue;
    if(shape.code==3){if(count!=25)continue;data=vector+4;}
    else if(!mem::Read(vector+shape.data,data)||(data&&!mem::ValidPtr(data))||(count&&!data))continue;
    // Alternate private vector layout requires proof this inventory belongs to this full handle.
    uint32_t parent=0;bool parentRead=mem::Read(inventory+off::Inventory::m_hInventoryParent,parent);
    if((parentRead&&parent!=unit.entityHandle)||(shape.code>=2&&!parentRead))continue;
    ItemInfo pending[27]{};int n=0,resolved=0,unmapped=0;bool complete=true;
    for(int slot=0;slot<count;++slot) {
        uint32_t handle=0;
        if(!mem::Read(data+4ULL*slot,handle)){complete=false;break;}
        if(handle==0xFFFFFFFFu||handle==0)continue;
        uintptr_t item=ResolveIndexEx((int)(handle & g_sys.handleMask),handle);
        if(!mem::ValidPtr(item)){complete=false;continue;}
        uintptr_t itemIdentity=0,itemBacklink=0;uint32_t serial=0;
        if(!mem::Read(item+off::instEntity,itemIdentity)||!mem::ValidPtr(itemIdentity)||
           !mem::Read(itemIdentity,itemBacklink)||itemBacklink!=item||!mem::Read(itemIdentity+off::idHandleFld,serial)||serial!=handle){complete=false;continue;}
        ++resolved;uintptr_t vp=0;
        if(!mem::Read(item,vp)){complete=false;continue;}
        const char* cls=rtti::ClassOf(g_sys.clientBase,vp);
        ItemInfo& info=pending[n++];info.addr=item;info.instanceHandle=handle;info.slot=slot;
        const char* icon=MappedIcon(cls,true);
        if(icon)strncpy(info.icon,icon,sizeof(info.icon)-1);
        else if(!ItemIdentityName(item,info.icon,sizeof(info.icon)))++unmapped;
        int charges=-1;
        if(mem::Read(item+off::Item::m_iCurrentCharges,charges)&&charges>=0&&charges<=10000)info.charges=charges;
        float cd=-1.f;
        if(mem::Read(item+off::Ability::m_fCooldown,cd)&&std::isfinite(cd)&&cd>=0.f&&cd<=3600.f){info.cd=cd;info.cooldownRead=true;}
        info.cdLen=mem::ReadOr<float>(item+off::Ability::m_flCooldownLength,0.f);
        // ReclaimTime is NOT synthesized from purchase or first observation time.
        float expires=-1.f;
        if(mem::Read(item+off::Item::m_flReclaimTime,expires)&&std::isfinite(expires)&&expires>0.f)info.expiresAt=expires;
    }

    int checkCount=0,checkParity=0;uintptr_t checkData=0;
    checkData=shape.code==3?vector+4:0;
    if(!mem::Read(vector+shape.count,checkCount)||(shape.code!=3&&!mem::Read(vector+shape.data,checkData))||
       !mem::Read(inventory+off::Inventory::m_iParity,checkParity)||count!=checkCount||data!=checkData||parity!=checkParity)continue;
    // Inline handles can mutate without moving a pointer: reread every slot.
    if(shape.code==3){for(int slot=0;slot<count;++slot){uint32_t again=0;int found=-1;
       for(int i=0;i<n;++i)if(pending[i].slot==slot)found=i;
       if(!mem::Read(data+4ULL*slot,again)||(found>=0?again!=pending[found].instanceHandle:(again!=0&&again!=0xFFFFFFFFu))){complete=false;break;}}}
    if(parentRead){uint32_t parentAgain=0;if(!mem::Read(inventory+off::Inventory::m_hInventoryParent,parentAgain)||parentAgain!=parent)continue;}
    // Every non-empty slot must resolve: no partial inventory is allowed to drive Armlet.
    if(!complete)continue;
    if(matches){bool same=selected.inventoryCount==count&&selected.itemN==n;
       for(int i=0;same&&i<n;++i)same=selected.items[i].instanceHandle==pending[i].instanceHandle&&selected.items[i].slot==pending[i].slot;
       if(!same){unit.inventoryLayout=-1;return;}continue;}
    ++matches;selected.inventoryCount=count;selected.inventoryResolved=resolved;selected.inventoryUnmapped=unmapped;
    selected.inventoryRead=true;selected.inventoryLayout=shape.code;selected.itemN=n;
    for(int i=0;i<n;++i)selected.items[i]=pending[i];
    }
    if(matches){unit.inventoryCount=selected.inventoryCount;unit.inventoryResolved=selected.inventoryResolved;unit.inventoryUnmapped=selected.inventoryUnmapped;
       unit.inventoryRead=true;unit.inventoryLayout=selected.inventoryLayout;unit.itemN=selected.itemN;for(int i=0;i<unit.itemN;++i)unit.items[i]=selected.items[i];}
}

// Fallback is limited to schema-backed item ownership. Each candidate is
// revalidated on the render thread, including vptr and current owner handle.
// Whether this game's Aegis uses BaseEntity::OwnerEntity needs a live test;
// a failed owner check yields no fabricated Aegis or fabricated five-minute timer.
static void ReadOwnedAegis(uintptr_t npc,FrameUnit& unit) {
    for(int i=0;i<unit.itemN;++i)if(Streq(unit.items[i].icon,"aegis"))return;
    for(const auto& candidate:g_frameAegis){
        uintptr_t vptr=0;uint32_t owner=0;
        if(!mem::Read(candidate.addr,vptr)||vptr!=candidate.vptr||
           !mem::Read(candidate.addr+off::BaseEntity::m_hOwnerEntity,owner)||ResolveIndexEx((int)(owner & g_sys.handleMask),owner)!=npc)continue;
        if(unit.itemN>=27)return;
        ItemInfo& info=unit.items[unit.itemN++];info=ItemInfo{};info.addr=candidate.addr;info.slot=-2;
        uintptr_t identity=0;
        if(mem::Read(candidate.addr+off::EntityInstance::m_pEntity,identity)&&mem::ValidPtr(identity))
            mem::Read(identity+off::idHandleFld,info.instanceHandle);
        strcpy(info.icon,"aegis");info.cooldownRead=true;info.cd=0;
        float expires=-1.f;
        if(mem::Read(candidate.addr+off::Item::m_flReclaimTime,expires)&&std::isfinite(expires)&&expires>0.f)info.expiresAt=expires;
        return;
    }
}

static int ReadAbilities(uintptr_t npc, AbilityInfo* out, int cap) {

    int cnt = 0;
    uintptr_t vec = 0;
    mem::Read(npc + off::NPC::m_vecAbilities, cnt);
    mem::Read(npc + off::NPC::m_vecAbilities + 8, vec);
    if (cnt <= 0 || cnt > 32) return 0;
    if (vec < 0x100000000ULL || vec > 0x00007FFFFFFFFFFFULL) return 0;

    int n = 0;
    for (int i = 0; i < cnt && n < cap; ++i) {
        uint32_t h = 0;
        if (!mem::Read(vec + 4ULL * i, h)) continue;
        uintptr_t a = EntityByHandle(h);
        if (!mem::ValidPtr(a)) continue;

        int lvl = mem::ReadOr<int>(a + off::Ability::m_iLevel);
        if (lvl < 0 || lvl > 30) continue;
        bool hidden = true;
        if (!mem::Read(a + off::Ability::m_bHidden, hidden) || hidden) continue;
        uint32_t barType = 0xFFFFFFFFu;
        if (!mem::Read(a + off::Ability::m_nAbilityBarType, barType) || barType != 0) continue;
        uintptr_t avp = 0;
        if (!mem::Read(a, avp)) continue;
        const char* abilityClass = rtti::ClassOf(g_sys.clientBase, avp);
        const char* icon = MappedIcon(abilityClass, false);
        // No empty/unknown slots and no common secondary utility abilities.
        // Unmapped main abilities are excluded from this visual strip, not invented.
        if (!icon || !*icon || Contains(icon, "_innate") ||
            Streq(icon,"attribute_bonus") || Contains(icon,"high_five") || Contains(icon,"generic_hidden")) continue;

        AbilityInfo& ai = out[n++];
        ai = AbilityInfo{};ai.addr=a;ai.entityHandle=h;
        ai.slot = i;
        ai.level = lvl;
        ai.cls = abilityClass;
        ai.maxLevel = mem::ReadOr<int>(a + off::Ability::m_iMaxLevel, 0);
        int maxOverride = mem::ReadOr<int>(a + off::Ability::m_nMaxLevelOverride, 0);
        if (maxOverride > 0 && maxOverride <= 10) ai.maxLevel = maxOverride;
        if (ai.maxLevel < 1 || ai.maxLevel > 10) {
            // Do not guess 4 for every spell. Exact published fallback, no automation impact.
            int published=PublishedAbilityMax(icon);
            ai.maxLevel=(published>=lvl)?published:0;
        }
        bool manaRead=mem::Read(a+off::Ability::m_iManaCost,ai.mana)&&ai.mana>=0&&ai.mana<=20000;
        float cooldown = -1.f;
        ai.cooldownRead = mem::Read(a + off::Ability::m_fCooldown, cooldown) &&
                          std::isfinite(cooldown) && cooldown >= 0.f && cooldown <= 3600.f;
        uint8_t activated=0;bool activeRead=mem::Read(a+0x619,activated)&&activated==1;
        ai.automationReady=ai.cooldownRead&&cooldown<=.01f&&activeRead&&lvl>0&&manaRead;
        ai.cd = ai.cooldownRead ? cooldown : 0.f;
        ai.cdLen = mem::ReadOr<float>(a + off::Ability::m_flCooldownLength);
        uint8_t phase=2,frozen=2,indefinite=2;float muted=-1;
        bool phaseRead=mem::Read(a+off::Ability::m_bInAbilityPhase,phase)&&phase<=1;
        ai.phaseRead=phaseRead;ai.phase=phaseRead&&phase!=0;
        ai.automationReady=ai.automationReady&&phaseRead&&!ai.phase &&
            mem::Read(a+0x630,muted)&&std::isfinite(muted)&&muted==0 &&
            mem::Read(a+0x654,indefinite)&&indefinite==0&&mem::Read(a+0x655,frozen)&&frozen==0;
        strncpy(ai.icon, icon, sizeof(ai.icon) - 1);
        if (ai.cd < 0.f || ai.cd > 600.f) ai.cd = 0.f;
        if (ai.cdLen < 0.f || ai.cdLen > 900.f) ai.cdLen = 0.f;
    }
    return n;
}

static void Reset(Frame& f) {
    f.ok = false;
    f.observedOnly = false;
    f.now = 0.f;
    f.localTeam = 2;
    f.localPos = Vec3{};
    f.hp = f.maxHp = f.mana = f.maxMana = f.level = 0;
    f.dmgAvg = 0.f;
    f.damageMin=f.damageMax=f.damageBonus=0;f.damageRead=false;
    f.atkRange = 0;
    f.localAlive = false;
    f.rules = 0;f.localHero=0;f.localHandle=0;
    f.queryUnit = 0;
    f.gameStart = -1.f;
    f.gameState = -1;
    f.roshanPhase = -1;
    f.roshanEnd = -1.f;
    f.roshanAlive = false;
    f.roshanHp = f.roshanMaxHp = 0;
    f.roshanPos = Vec3{};
    f.roshanPosValid = false;
    f.roshanEtaLo = f.roshanEtaHi = -1.f;
    f.lastHitDeny = cfg::deny;
    f.units.clear();
}

static float ArmorMult(float a) {
    return 1.0f - (0.06f * a) / (1.0f + 0.06f * fabsf(a));
}

static bool ReadOrigin(uintptr_t e, Vec3& out) {
    uintptr_t node = 0;
    if (!mem::Read(e + off::BaseEntity::m_pGameSceneNode, node) || node < 0x10000ULL) return false;
    return mem::Read(node + off::SceneNode::m_vecAbsOrigin, out);
}

struct BuffMemoryReader {
 uintptr_t npc;
 bool Valid(uintptr_t p){return mem::ValidPtr(p);}
 template<class T>bool Read(uintptr_t a,T& v){return mem::Read(a,v);}
 bool String(uintptr_t a,char* v,int n){return mem::ReadStr(a,v,n);}
 bool Owner(uint32_t h){return EntityByHandle(h)==npc;}
 bool BuffClass(uintptr_t vp){const char* c=rtti::ClassOf(g_sys.clientBase,vp);
  return c && (StartsWith(c,"CDOTA_Modifier")||StartsWith(c,"CDOTA_Modifer")||StartsWith(c,"CDOTA_Buff")||StartsWith(c,"CDOTA_Ability_"));}
};
static std::unordered_map<uint32_t,buffreader::Layout> buffLayouts;
static void ReadBuffs(uintptr_t npc,FrameUnit& u,float now){
 if(!u.entityHandle || EntityByHandle(u.entityHandle)!=npc)return;
 if(buffLayouts.size()>128)buffLayouts.clear();
 BuffMemoryReader reader{npc};auto& layout=buffLayouts[u.entityHandle];
 auto result=buffreader::Read(reader,npc+off::NPC::m_ModifierManager,u.entityHandle,now,layout);
 u.buffsRead=result.verified;u.buffsVisualRead=result.verified||result.visualOnly;u.buffs=std::move(result.buffs);
}

static bool TeamDataIdentity(uintptr_t object,int team,uint32_t& handle){
 uintptr_t vp=0,id=0,back=0;return mem::ValidPtr(object)&&mem::Read(object,vp)&&
 Streq(rtti::ClassOf(g_sys.clientBase,vp),team==2?"C_DOTA_DataRadiant":"C_DOTA_DataDire")&&
 mem::Read(object+off::instEntity,id)&&mem::ValidPtr(id)&&mem::Read(id,back)&&back==object&&
 mem::Read(id+off::idHandleFld,handle)&&handle&&handle!=0xffffffffu&&EntityByHandle(handle)==object;
}
static void ReadNPCVisibility(FrameUnit& unit,const Frame& f){
 unit.npcVisibilityRead=unit.npcVisible=false;unit.npcVisibilityProbeMask=0;unit.npcVisibilityWord=unit.npcVisibilitySelfWord=0;unit.npcVisibilityDataHandle=unit.npcVisibilityIndex=0;
 if(f.observedOnly||!f.localHandle||!f.localHero||(f.localTeam!=2&&f.localTeam!=3)||!unit.entityHandle)return;
 uintptr_t data=g_teamVisibilityData[f.localTeam-2].load();uint32_t expected=g_teamVisibilityDataHandle[f.localTeam-2].load();uint32_t h=0,h2=0,assigned=0,assigned2=0;uint8_t localFlag=2,localFlag2=2;
 if(!expected||!TeamDataIdentity(data,f.localTeam,h)||h!=expected||EntityByHandle(f.localHandle)!=f.localHero||EntityByHandle(unit.entityHandle)!=unit.addr||
    !mem::Read(g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned)||assigned!=f.localHandle||
    !mem::Read(g_sys.localCtrl+off::Ctrl::m_bIsLocalPlayerController,localFlag)||localFlag!=1)return;
 // Raw bounded candidates are diagnostic only; known/visible remain false until all stable checks pass.
 uint32_t localIndex=f.localHandle&g_sys.handleMask,targetIndex=unit.entityHandle&g_sys.handleMask;
 unit.npcVisibilityDataHandle=h;unit.npcVisibilityIndex=targetIndex;
 if(g_sys.handleMask==0x3fffu&&localIndex<16384&&targetIndex<16384){
  if(mem::Read(data+off::TeamVisibilityData::m_bNPCVisibleState+8*(localIndex/64),unit.npcVisibilitySelfWord))unit.npcVisibilityProbeMask|=1;
  if(mem::Read(data+off::TeamVisibilityData::m_bNPCVisibleState+8*(targetIndex/64),unit.npcVisibilityWord))unit.npcVisibilityProbeMask|=2;
 }
 struct Reader{bool Read(uintptr_t a,uint64_t& v){return mem::Read(a,v);}} reader;
 auto bits=npcvisibility::Read(reader,data+off::TeamVisibilityData::m_bNPCVisibleState,unit.entityHandle,f.localHandle,g_sys.handleMask);
 if(!bits.known||!TeamDataIdentity(data,f.localTeam,h2)||h2!=h||g_teamVisibilityDataHandle[f.localTeam-2].load()!=expected||g_teamVisibilityData[f.localTeam-2].load()!=data||EntityByHandle(unit.entityHandle)!=unit.addr||EntityByHandle(f.localHandle)!=f.localHero||
    !mem::Read(g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned2)||assigned2!=assigned||!mem::Read(g_sys.localCtrl+off::Ctrl::m_bIsLocalPlayerController,localFlag2)||localFlag2!=localFlag)return;
 unit.npcVisibilityRead=true;unit.npcVisible=bits.visible;unit.npcVisibilityWord=bits.word;unit.npcVisibilitySelfWord=bits.selfWord;unit.npcVisibilityIndex=bits.index;unit.npcVisibilityDataHandle=h;
}
static bool FillUnit(const StaticUnit& u, FrameUnit& fu, const Frame& f) {
    // Reject a snapshot from a previous occupant of this address.
    if (u.entityHandle && EntityByHandle(u.entityHandle) != u.addr) return false;
    fu = FrameUnit{};
    fu.addr = u.addr;
    fu.kind = u.kind;
    fu.cls = u.cls;
    memcpy(fu.name, u.name, sizeof(fu.name));
    memcpy(fu.nick, u.nick, sizeof(fu.nick));

    if (!ReadOrigin(u.addr, fu.pos) || !std::isfinite(fu.pos.x) || !std::isfinite(fu.pos.y) ||
        !std::isfinite(fu.pos.z) || fabsf(fu.pos.x)>50000 || fabsf(fu.pos.y)>50000) return false;
    uintptr_t identity=0;
    if (mem::Read(u.addr+off::instEntity,identity)) mem::Read(identity+off::idHandleFld,fu.entityHandle);
    if (u.kind==UnitKind::Rune || u.kind==UnitKind::RuneSpawner) {
        if (!fu.entityHandle || ResolveIndexEx((int)(fu.entityHandle&g_sys.handleMask),fu.entityHandle)!=u.addr) return false;
        uintptr_t vp=0; if (!mem::Read(u.addr,vp) || !Streq(rtti::ClassOf(g_sys.clientBase,vp),u.cls)) return false;
        if (u.kind==UnitKind::Rune) {
            fu.runeRead=mem::Read(u.addr+off::Rune::type,fu.runeType) && fu.runeType>=0 && fu.runeType<10;
            // Schema does not establish whether RuneTime is creation or expiry.
            // Keep it as optional metadata; never infer a spawn from it.
            mem::Read(u.addr+off::Rune::time,fu.runeTime);
        } else {
            fu.runeRead=mem::Read(u.addr+off::RuneSpawner::type,fu.runeType) &&
                        mem::Read(u.addr+off::RuneSpawner::last,fu.runeLastSpawn) &&
                        mem::Read(u.addr+off::RuneSpawner::next,fu.runeNextSpawn) &&
                        fu.runeType>=-1 && fu.runeType<10 && std::isfinite(fu.runeLastSpawn) &&
                        std::isfinite(fu.runeNextSpawn) && fu.runeLastSpawn>=-120.f &&
                        fu.runeLastSpawn<=f.now+1.f && fu.runeNextSpawn>=-1.f && fu.runeNextSpawn<=f.now+3600.f;
            fu.nextRuneType=fu.runeType;
            if (Streq(u.cls,"C_DOTA_Item_RuneSpawner_Powerup")) {
                uint8_t water=2; if(!mem::Read(u.addr+off::RuneSpawner::water,water)||water>1)fu.runeRead=false;
                fu.nextRuneType=water==1?7:-1; // Future random rune is not known.
            }
            if (Streq(u.cls,"C_DOTA_Item_RuneSpawner_XP"))fu.nextRuneType=8;
            if (Streq(u.cls,"C_DOTA_Item_RuneSpawner_Bounty"))fu.nextRuneType=5;
        }
        fu.alive=fu.runeRead; return fu.runeRead;
    }
    if (!mem::Read(u.addr + off::BaseEntity::m_iHealth, fu.hp)) return false;
    fu.maxHp = mem::ReadOr<int>(u.addr + off::BaseEntity::m_iMaxHealth);
    if (fu.maxHp <= 0) return false;

    uint8_t team = 0;
    mem::Read(u.addr + off::BaseEntity::m_iTeamNum, team);
    fu.team = team;

    uint8_t life = 1;
    mem::Read(u.addr + off::BaseEntity::m_lifeState, life);
    fu.alive = (life == 0 && fu.hp > 0);

    uint8_t illusion=2;
    fu.illusionRead=mem::Read(u.addr+off::NPC::m_bIsIllusion,illusion)&&illusion<=1;
    fu.illusion=fu.illusionRead&&illusion!=0;
    fu.invisRead=mem::Read(u.addr+off::NPC::m_flInvisibilityLevel,fu.invis)&&std::isfinite(fu.invis)&&fu.invis>=0.f&&fu.invis<=1.f;
    if(!fu.invisRead)fu.invis=0.f;
    fu.stateRead=mem::Read(u.addr+off::NPC::m_nUnitState64,fu.unitState);
    if(!fu.stateRead)fu.unitState=0;
    fu.clockRead=mem::Read(u.addr+off::BaseEntity::m_flSimulationTime,fu.sampleTime)&&std::isfinite(fu.sampleTime)&&fu.sampleTime>0.f&&fu.sampleTime<86400.f;
    if(!fu.clockRead)fu.clockRead=mem::Read(u.addr+off::BaseEntity::m_flAnimTime,fu.sampleTime)&&std::isfinite(fu.sampleTime)&&fu.sampleTime>0.f&&fu.sampleTime<86400.f;
    fu.level = mem::ReadOr<int>(u.addr + off::NPC::m_iCurrentLevel, 0);
    fu.manaRead=mem::Read(u.addr+off::NPC::m_flMana,fu.mana)&&mem::Read(u.addr+off::NPC::m_flMaxMana,fu.maxMana)&&
        std::isfinite(fu.mana)&&std::isfinite(fu.maxMana)&&fu.mana>=0&&fu.maxMana>=0&&fu.mana<=100000&&fu.maxMana<=100000;
    if(!fu.manaRead)fu.mana=fu.maxMana=0.f;

    int hbo = mem::ReadOr<int>(u.addr + off::NPC::m_iHealthBarOffset, 0);
    fu.hbOffset = (hbo > 0 && hbo < 1500) ? (float)hbo : 200.f;

    fu.dist = VecDist(fu.pos, f.localPos);

    if (fu.kind == UnitKind::Creep) {
        int lo = -1, hi = -1;
        if (mem::Read(u.addr + off::NPC::m_iGoldBountyMin, lo) &&
            mem::Read(u.addr + off::NPC::m_iGoldBountyMax, hi) &&
            lo >= 0 && hi >= lo && hi <= 100000) {
            fu.bountyMin = lo;
            fu.bountyMax = hi;
        }
    }
    if(f.observedOnly) {
        // Inventory, statuses and per-entity clock are independent of local player identity.
        // Visual reads do NOT make this a trusted local frame or enable any automation.
        if(!fu.entityHandle||fu.hp<0||fu.hp>fu.maxHp||fu.maxHp>10000000||
           (fu.team!=2&&fu.team!=3&&fu.team!=4))return false;
        fu.dist=0.f;
        if(fu.kind==UnitKind::Hero) {
            fu.playerId=mem::ReadOr<int>(u.addr+off::Hero::m_iPlayerID,-1);
            if(cfg::hudItems)ReadItems(u.addr,fu);
            if(cfg::hudAbilities)fu.abilN=ReadAbilities(u.addr,fu.abil,16);
            if((cfg::showEffects||cfg::hudStatusBadges||cfg::hudIllusions)&&fu.clockRead)ReadBuffs(u.addr,fu,fu.sampleTime);
            // No global cooldown-direction guesses, last-hit data, respawn or Aegis clock inferred.
        }
        return true;
    }
    if (fu.kind == UnitKind::Hero || fu.kind == UnitKind::Ward){
        fu.teamVisibilityRead = mem::Read(u.addr + off::ModelEntity::m_iTeamVisibilityBitmask, fu.teamVisibilityMask);
        ReadNPCVisibility(fu,f);
        uintptr_t node=0,owner=0;uint8_t dormant=2;
        fu.sceneDormantRead=mem::Read(u.addr+off::BaseEntity::m_pGameSceneNode,node)&&mem::ValidPtr(node)&&mem::Read(node+off::SceneNode::m_pOwner,owner)&&owner==u.addr&&mem::Read(node+off::SceneNode::m_bDormant,dormant)&&dormant<=1;
        fu.sceneDormant=fu.sceneDormantRead&&dormant!=0; // diagnostics only, NOT team vision
    }
    if (fu.kind == UnitKind::Hero) {
        if(cfg::showEffects || cfg::hudStatusBadges || cfg::hudIllusions || cfg::killStealer || cfg::showKillHelper || cfg::armletAuto)ReadBuffs(u.addr,fu,f.now);
        fu.playerId = mem::ReadOr<int>(u.addr + off::Hero::m_iPlayerID, -1);
        ReadItems(u.addr, fu);
        ReadOwnedAegis(u.addr, fu);
        float v = mem::ReadOr<float>(u.addr + off::Hero::m_flRespawnTime, -1.f);
        if (!fu.alive) {
            float rem = v - f.now;
            if (rem < -1.f) {
                float dt = mem::ReadOr<float>(u.addr + off::NPC::m_flDeathTime, 0.f);
                rem = (dt + v) - f.now;
            }
            if (rem < 0.f) rem = 0.f;
            if (rem > 900.f) rem = 0.f;
            fu.respawn = (int)(rem + 0.5f);
        }
        fu.abilN = ReadAbilities(u.addr, fu.abil, 16);

        float sim = mem::ReadOr<float>(u.addr + off::BaseEntity::m_flSimulationTime, -1.f);
        if (!(sim > 0.f))
            sim = mem::ReadOr<float>(u.addr + off::BaseEntity::m_flAnimTime, -1.f);
        MoveHist& h = g_moveHist[u.addr];
        if (sim > 0.f && sim != h.sim) {
            if (h.sim > 0.f) {
                float dx = fu.pos.x - h.pos.x, dy = fu.pos.y - h.pos.y;
                float d2 = dx * dx + dy * dy;
                if (d2 > 16.f && d2 < 360000.f) {
                    float d = sqrtf(d2);
                    h.dir = Vec3{ dx / d, dy / d, 0.f };
                    h.ok = true;
                } else {
                    h.ok = false;
                }
            }
            h.pos = fu.pos;
            h.sim = sim;
        }
        if (h.ok) {
            fu.mdirOk = true;
            fu.mdir = h.dir;
        }
    }

    bool target = (fu.kind == UnitKind::Creep || fu.kind == UnitKind::Building || fu.kind == UnitKind::Boss);
    if (target && f.localAlive) {
        float armor=0.f;
        bool armorRead=mem::Read(u.addr+off::NPC::m_flPhysicalArmorValue,armor);
        fu.armor=armor;
        auto damage=damageestimate::Physical(f.damageMin,f.damageMax,f.damageBonus,armor);
        damage.valid=damage.valid&&f.damageRead&&armorRead;
        bool lethal=damageestimate::Lethal(fu.hp,damage,cfg::lastHitConservative);
        bool inRange=fu.dist<=(float)f.atkRange; // Do not add a fictitious 75-unit reach.
        if(inRange&&fu.team!=f.localTeam) {
            fu.canLastHit=lethal;
            if(damage.valid)fu.hits=(int)ceilf(fu.hp/std::max(1.f,damage.low));
        }
        if(inRange&&fu.kind==UnitKind::Creep&&fu.team==f.localTeam&&f.lastHitDeny)
            fu.canDeny=fu.hp<fu.maxHp*.5f&&lethal;
    }
    return true;
}

void BuildObservedFrame(Frame& f) {
    Reset(f);
    if (!g_sys.ready) return;
    f.observedOnly=true;
    f.localTeam=0; // Unknown. Do not silently assume Radiant or choose a hero as 'self'.
    CopyUnits(g_frameSnapshot);
    f.units.reserve(g_frameSnapshot.size());
    for (const auto& u:g_frameSnapshot) {
        if(u.kind==UnitKind::Rune||u.kind==UnitKind::RuneSpawner)continue; // no trusted clock
        FrameUnit out;
        if(FillUnit(u,out,f))f.units.push_back(out);
    }
    // Intentionally keep ok/localAlive/localHero/localHandle false/zero.
}

static bool  s_roshanAlivePrev = false;
static float s_roshanDiedAt = -1.f;

void BuildFrame(Frame& f, uintptr_t ctrl) {
    Reset(f);
    auto& probe = g_controllerProbe;
    probe.frameStage.store(1);probe.heroHandle.store(0);probe.heroAddress.store(0);
    if (!g_sys.ready) return;
    probe.frameStage.store(2);
    if (!mem::ValidPtr(ctrl)) return;
    probe.frameStage.store(3);
    uintptr_t vp = 0;
    if (!mem::Read(ctrl, vp) || !IsPlayerControllerVtable(vp) ||
        !ControllerCandidate(ctrl, probe.source.load()==1 && probe.globalValue.load()==ctrl)) return;
    probe.frameStage.store(4);
    if (!RefreshPages()) return;
    probe.frameStage.store(5);
    uint32_t heroH = 0;
    if (!mem::Read(ctrl + off::Ctrl::m_hAssignedHero, heroH) || !heroH || heroH == 0xFFFFFFFFu) return;
    probe.heroHandle.store(heroH);
    uintptr_t hero = EntityByHandle(heroH);
    if (!hero && DetectMask(ctrl)) hero = EntityByHandle(heroH);
    probe.heroAddress.store(hero);probe.frameStage.store(6);
    if (!IsHeroEntity(hero)) return;
    probe.frameStage.store(11);
    int playerId = -1;
    if (!mem::Read(ctrl + off::Ctrl::m_nPlayerID, playerId) || playerId < 0 || playerId >= 64 ||
        !HeroBelongsToPlayer(hero,playerId)) return;
    f.localHero=hero;f.localHandle=heroH;

    f.now = mem::ReadOr<float>(hero + off::BaseEntity::m_flSimulationTime, 0.f);
    if (!(f.now > 0.f))
        f.now = mem::ReadOr<float>(hero + off::BaseEntity::m_flAnimTime, 0.f);
    uint8_t localTeam = 0;
    bool localTeamRead = mem::Read(hero + off::BaseEntity::m_iTeamNum, localTeam);
    f.localTeam = localTeam;
    f.hp = mem::ReadOr<int>(hero + off::BaseEntity::m_iHealth, 0);
    f.maxHp = mem::ReadOr<int>(hero + off::BaseEntity::m_iMaxHealth, 0);
    f.mana = (int)mem::ReadOr<float>(hero + off::NPC::m_flMana, 0.f);
    f.maxMana = (int)mem::ReadOr<float>(hero + off::NPC::m_flMaxMana, 0.f);
    f.level = mem::ReadOr<int>(hero + off::NPC::m_iCurrentLevel, 0);

    uint8_t life = 1;
    mem::Read(hero + off::BaseEntity::m_lifeState, life);
    f.localAlive = (life == 0 && f.hp > 0);

    int dmin=0,dmax=0,dbon=0;
    f.damageRead=mem::Read(hero+off::NPC::m_iDamageMin,dmin) &&
        mem::Read(hero+off::NPC::m_iDamageMax,dmax) && mem::Read(hero+off::NPC::m_iDamageBonus,dbon) &&
        damageestimate::Physical(dmin,dmax,dbon,0.f).valid;
    f.damageMin=dmin;f.damageMax=dmax;f.damageBonus=dbon;
    f.dmgAvg = ((float)dmin + (float)dmax) * 0.5f + (float)dbon;
    if (f.dmgAvg <= 0.f) f.dmgAvg = (float)(dmin + dbon);

    f.atkRange = mem::ReadOr<int>(hero + off::NPC::m_iAttackRange, 0);
    if (f.atkRange <= 0) f.atkRange = 300;

    probe.frameStage.store(7);
    if (!ReadOrigin(hero, f.localPos) || !std::isfinite(f.localPos.x) ||
        !std::isfinite(f.localPos.y) || !std::isfinite(f.localPos.z) ||
        fabsf(f.localPos.x)>50000 || fabsf(f.localPos.y)>50000) return;
    probe.frameStage.store(8);
    if (!localTeamRead || (f.localTeam != 2 && f.localTeam != 3) || f.maxHp <= 0 || f.maxHp > 10000000 ||
        f.hp < 0 || f.hp > f.maxHp) return;
    probe.frameStage.store(9);
    if (!std::isfinite(f.now) || f.now < 0.f || f.now > 300000.f) return;

    f.rules = g_rules;
    uint32_t query = 0;
    if (mem::Read(ctrl + off::CtrlHUD::m_hQueryUnit, query)) f.queryUnit = EntityByHandle(query);
    if (g_rules) {
        f.gameState = mem::ReadOr<int>(g_rules + off::Rules::m_nGameState, -1);
        f.gameStart = mem::ReadOr<float>(g_rules + off::Rules::m_flGameStartTime, -1.f);
        f.roshanPhase = (int)mem::ReadOr<uint32_t>(g_rules + off::Rules::m_nRoshanRespawnPhase, 99);
        f.roshanEnd = mem::ReadOr<float>(g_rules + off::Rules::m_flRoshanRespawnPhaseEndTime, -1.f);
    }
    if (g_dataEnt) {
        uintptr_t rp = g_dataEnt + off::Data::m_roshanSpawnInfo;
        uint32_t ph = 99;
        mem::Read(rp + off::RoshanPhase::m_eRoshanPhase, ph);
        if (ph <= 2) {
            f.roshanPhase = (int)ph;
            f.roshanEnd = mem::ReadOr<float>(rp + off::RoshanPhase::m_flRoshanPhaseEndTime, f.roshanEnd);
        }
    }

    static uintptr_t previousBuffRules=0;static float previousBuffStart=-1,previousBuffNow=-1;
    if(previousBuffRules!=f.rules||previousBuffStart!=f.gameStart||f.now<previousBuffNow-.25f)buffLayouts.clear();
    previousBuffRules=f.rules;previousBuffStart=f.gameStart;previousBuffNow=f.now;
    CopyUnits(g_frameSnapshot);

    f.units.reserve(g_frameSnapshot.size());
    for (size_t i = 0; i < g_frameSnapshot.size(); ++i) {
        FrameUnit fu;
        if (FillUnit(g_frameSnapshot[i], fu, f)) f.units.push_back(fu);
    }

    // ESP20: m_fCooldown is read as remaining seconds per ability/item.
    // Never invert all cooldowns based on other heroes' ready/full vote counts.

    bool seen = false;
    for (auto& u : f.units) {
        if (u.kind != UnitKind::Roshan) continue;
        seen = true;
        f.roshanAlive = u.alive;
        f.roshanHp = u.hp;
        f.roshanMaxHp = u.maxHp;
        f.roshanPos = u.pos;
        f.roshanPosValid = true;
        g_roshanLastPos = u.pos;
        g_roshanSeen = true;
    }

    if (seen) {
        if (!f.roshanAlive && s_roshanAlivePrev) s_roshanDiedAt = f.now;
        s_roshanAlivePrev = f.roshanAlive;
    } else {
        s_roshanAlivePrev = false;
        f.roshanPos = g_roshanLastPos;
        f.roshanPosValid = g_roshanSeen;
    }

    if (!f.roshanAlive) {
        bool exact = (f.roshanPhase > 0 && f.roshanEnd > 0.f && f.roshanEnd > f.now - 1.f);
        if (exact) {
            f.roshanEtaLo = f.roshanEtaHi = f.roshanEnd - f.now;
        } else if (s_roshanDiedAt > 0.f) {
            float elapsed = f.now - s_roshanDiedAt;
            f.roshanEtaLo = 480.f - elapsed;
            f.roshanEtaHi = 660.f - elapsed;
            if (f.roshanEtaHi < 0.f) { f.roshanEtaLo = 0.f; f.roshanEtaHi = 0.f; }
        }
    } else {
        s_roshanDiedAt = -1.f;
    }

    f.ok = true;
    probe.frameStage.store(10);
}

}
