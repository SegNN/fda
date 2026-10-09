#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace cfg {
    inline bool menuOpen  = true;
    inline bool armletAuto=false, armletConfirmed=false;
    inline int armletKey=0, armletSlot=0;
    inline float armletThreshold=350.f;
    inline bool hudTop = true;
    inline bool hudItems = true;
    inline bool hudAbilities = true;
    inline bool hudRoshan = true;
    inline bool showEffects=true, effectsTimedOnly=true;
    inline bool showKillHelper=true, helperModeledOnly=true;
    inline float helperMargin=30.f;
    inline bool ksHoldActive=false;
    inline bool killStealer=false, ksQuickcastConfirmed=false;
    inline float ksDamage=0.f, ksRange=0.f, ksMargin=30.f;
    inline char ksHeroName[64]={};
    inline int ksAbilitySlot=3, ksSpellKey=0, ksDamageType=0;
    inline bool lastHitConservative = true;
    inline bool notifyRunes = true, notifyRuneSoon = true;
    inline bool notifyWards = false, notifyRoshan = true;
    inline float notifyLead = 10.f;
    inline bool hudWatermark = true;
    inline bool hudBounty = true;
    inline bool hudPortraits = true;
    inline bool hudHpNumber = true;
    inline bool hudManaNumber = true;
    inline bool hudIllusions = true;
    inline float hudIconScale = 1.f;
    inline float hudSkillPixels = 30.f;
    inline float nativeHpOffsetX=0.f,nativeHpOffsetY=-9.f;
    inline float nativeHpRedOffsetY=6.f;
    inline bool hudTopAbilities = true;
    inline bool hudStatusBadges = true;
    inline bool hudVisibleByEnemy = false; // experimental read-only client visibility mask
    inline bool farmHarass = false;
    inline bool lastHitEnabled = true;
    inline float hudTopY = 54.f;
    inline float hudTopSlot = 72.f;
    inline float hudTopGap = 184.f;
    inline float hudWatermarkX = -1.f, hudWatermarkY = 12.f;
    inline float hudRoshanX = -1.f, hudRoshanY = 220.f;
    inline char hudNickname[48] = "player"; // user-entered, not inferred Steam name


    inline bool espHeroes = true;
    inline bool espBoxes  = false; // Legacy Roshan-only setting, not a hero renderer.
    inline bool espCds    = true;
    inline bool espDist   = true;

    inline bool skillPreview = true;

    inline bool espWards  = true;
    inline bool espRoshan = true;

    inline bool lastHit   = true;
    inline bool deny      = true;

    inline bool vbe       = false;
    inline bool glow      = false;

    inline bool dotaPlus  = true;

    inline bool dodger    = true;
    inline bool autoDodge = false;
    inline bool farmBot   = false;
    inline bool farmAuto  = false;

    inline bool autoAccept   = false;
    inline bool showKeybinds = true;

    inline int  menuKey   = VK_INSERT;
    inline int  unloadKey = VK_END;

    inline int  rebindIdx = -1;

    inline std::atomic<bool> running{ true };
}

struct KeyBind {
    const char* name = "";
    bool* target = nullptr;
    int   vk = 0;
    bool  hold = false;
    bool  holdSaved = false;
    bool  holding = false;
};

namespace binds {

inline KeyBind items[] = {
    { "Last-Hit",    &cfg::farmBot, 'V', true },
    { "Auto-attack", &cfg::farmAuto },
    { "Auto-accept", &cfg::autoAccept },
    { "Last hit",    &cfg::lastHit },
    { "ESP heroes",  &cfg::espHeroes },
    { "Wards",       &cfg::espWards },
    { "Visible flag", &cfg::hudVisibleByEnemy },
    { "Purple glow", &cfg::glow },
    { "Auto dodge",  &cfg::autoDodge },
    { "Dota Plus",   &cfg::dotaPlus },
    { "Kill Stealer", &cfg::ksHoldActive, 0, true },
    { "Huskar Armlet", &cfg::armletAuto },
};

inline constexpr int kCount = (int)(sizeof(items) / sizeof(items[0]));

inline bool Edge[256] = {};
inline bool Down[256] = {};

void ProcessBinds();

inline const char* KeyName(int vk) {
    static char ring[4][8];
    static int rot = 0;
    char* out = ring[rot = (rot + 1) & 3];

    if (vk <= 0 || vk > 255) { snprintf(out, 8, "-"); return out; }

    if (vk >= '0' && vk <= '9') { snprintf(out, 8, "%c", vk); return out; }
    if (vk >= 'A' && vk <= 'Z') { snprintf(out, 8, "%c", vk); return out; }

    if (vk >= VK_F1 && vk <= VK_F24) { snprintf(out, 8, "F%d", vk - VK_F1 + 1); return out; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { snprintf(out, 8, "NUM%d", vk - VK_NUMPAD0); return out; }

    switch (vk) {
    case VK_LBUTTON:   return "M1";
    case VK_RBUTTON:   return "M2";
    case VK_MBUTTON:   return "M3";
    case VK_XBUTTON1:  return "M4";
    case VK_XBUTTON2:  return "M5";
    case VK_BACK:      return "BKSP";
    case VK_TAB:       return "TAB";
    case VK_RETURN:    return "ENTER";
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:    return "SHIFT";
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:  return "CTRL";
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:     return "ALT";
    case VK_PAUSE:     return "PAUSE";
    case VK_CAPITAL:   return "CAPS";
    case VK_ESCAPE:    return "ESC";
    case VK_SPACE:     return "SPACE";
    case VK_PRIOR:     return "PGUP";
    case VK_NEXT:      return "PGDN";
    case VK_END:       return "END";
    case VK_HOME:      return "HOME";
    case VK_LEFT:      return "LEFT";
    case VK_RIGHT:     return "RIGHT";
    case VK_UP:        return "UP";
    case VK_DOWN:      return "DOWN";
    case VK_INSERT:    return "INS";
    case VK_DELETE:    return "DEL";
    case VK_SNAPSHOT:  return "PRTSC";
    case VK_SCROLL:    return "SCRLK";
    case VK_MULTIPLY:  return "NUM*";
    case VK_ADD:       return "NUM+";
    case VK_SUBTRACT:  return "NUM-";
    case VK_DECIMAL:   return "NUM.";
    case VK_DIVIDE:    return "NUM/";
    case VK_OEM_1:     return ";";
    case VK_OEM_PLUS:  return "=";
    case VK_OEM_COMMA: return ",";
    case VK_OEM_MINUS: return "-";
    case VK_OEM_PERIOD:return ".";
    case VK_OEM_2:     return "/";
    case VK_OEM_3:     return "`";
    case VK_OEM_4:     return "[";
    case VK_OEM_5:     return "\\";
    case VK_OEM_6:     return "]";
    case VK_OEM_7:     return "'";
    default:           break;
    }
    snprintf(out, 8, "K%d", vk);
    return out;
}

}

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

inline float VecDist(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

inline bool Streq(const char* a, const char* b) { return a && b && strcmp(a, b) == 0; }

inline bool StartsWith(const char* s, const char* p) {
    if (!s || !p) return false;
    return strncmp(s, p, strlen(p)) == 0;
}

inline bool Contains(const char* s, const char* p) {
    if (!s || !p) return false;
    return strstr(s, p) != nullptr;
}
