// ============================================================================
//  Umbrella menu — 1:1 Dear ImGui replica of "Heroes Overlay" menu
//  All geometry is expressed in design units (screenshot px / 2) * S
// ============================================================================
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome6.h"
#include "font_data.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <algorithm>

// ---------------------------------------------------------------------------
// Palette / themes
// ---------------------------------------------------------------------------
struct Pal
{
    ImU32 accent;
    ImU32 rail, side, content, border1, border2, outer;
    ImU32 card, cardBorder, sep, sel, combo, comboHover;
    ImU32 text, textBright, title, sideText, railIcon, crumb, hdrIcon, placeholder, iconOff;
    ImU32 offTrack, knobOff, sliderBg, knob;
    ImU32 popup, popupBorder, popupSep, keyBox, keyText;
    ImU32 win, winBorder, searchBorder, backdropTop, backdropBottom;
    bool  light;
};
static Pal P;

struct ThemeDef { ImU32 swatchBg; ImU32 accent; ImVec4 tint; bool light; };
static ThemeDef gThemes[6] = {
    { IM_COL32(16, 17, 21, 255),   IM_COL32(207, 54, 54, 255),  ImVec4(1, 2, 6, 0),  false },
    { IM_COL32(230, 230, 230, 255),IM_COL32(207, 54, 54, 255),  ImVec4(0, 0, 0, 0),  true  },
    { IM_COL32(18, 10, 10, 255),   IM_COL32(239, 59, 59, 255),  ImVec4(0, 0, 0, 0),  false },
    { IM_COL32(14, 15, 22, 255),   IM_COL32(224, 94, 142, 255), ImVec4(-1, 0, 6, 0), false },
    { IM_COL32(6, 6, 6, 255),      IM_COL32(118, 118, 118, 255),ImVec4(-6,-6,-6, 0), false },
    { IM_COL32(14, 16, 20, 255),   IM_COL32(125, 158, 203, 255),ImVec4(-1, 1, 5, 0), false },
};

static ImU32 RGB(int r, int g, int b, int a = 255) { return IM_COL32(ImClamp(r, 0, 255), ImClamp(g, 0, 255), ImClamp(b, 0, 255), a); }
static ImU32 Tint(int r, int g, int b, const ImVec4& t) { return RGB(r + (int)t.x, g + (int)t.y, b + (int)t.z); }
static ImU32 LerpC(ImU32 a, ImU32 b, float t)
{
    ImVec4 A = ImGui::ColorConvertU32ToFloat4(a), B = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImLerp(A, B, ImSaturate(t)));
}
static ImU32 WithA(ImU32 c, float a)
{
    unsigned al = (unsigned)(((c >> 24) & 255) * ImSaturate(a));
    return (c & 0x00FFFFFF) | (al << 24);
}

static void BuildPalette(int theme, ImU32 custom_accent)
{
    ThemeDef td = theme < 6 ? gThemes[theme] : ThemeDef{ IM_COL32(15, 15, 15, 255), custom_accent, ImVec4(0, 0, 0, 0), false };
    const ImVec4& t = td.tint;
    P.light = td.light;
    P.accent = td.accent;
    if (!td.light)
    {
        P.rail = Tint(16, 16, 17, t);     P.side = Tint(19, 19, 20, t);     P.content = Tint(9, 9, 9, t);
        P.border1 = Tint(32, 32, 37, t);  P.border2 = Tint(26, 26, 29, t);  P.outer = Tint(30, 30, 34, t);
        P.card = Tint(15, 15, 15, t);     P.cardBorder = Tint(26, 26, 26, t); P.sep = Tint(27, 27, 30, t);
        P.sel = Tint(22, 28, 30, t);      P.combo = Tint(22, 25, 28, t);    P.comboHover = Tint(30, 33, 37, t);
        P.text = RGB(175, 175, 175);      P.textBright = RGB(245, 245, 245); P.title = RGB(142, 142, 142);
        P.sideText = RGB(150, 153, 163);  P.railIcon = RGB(204, 207, 221);  P.crumb = RGB(84, 89, 100);
        P.hdrIcon = RGB(109, 109, 109);   P.placeholder = RGB(111, 111, 111); P.iconOff = RGB(102, 102, 102);
        P.offTrack = Tint(23, 24, 27, t); P.knobOff = RGB(102, 102, 102);   P.sliderBg = Tint(35, 38, 45, t);
        P.knob = RGB(255, 255, 255);
        P.popup = Tint(22, 22, 22, t);    P.popupBorder = Tint(38, 38, 38, t); P.popupSep = Tint(31, 31, 33, t);
        P.keyBox = Tint(36, 39, 44, t);   P.keyText = RGB(172, 172, 172);
        P.win = Tint(10, 10, 10, t);      P.winBorder = Tint(26, 26, 26, t); P.searchBorder = Tint(23, 23, 23, t);
        P.backdropTop = Tint(28, 24, 26, t); P.backdropBottom = Tint(7, 7, 9, t);
    }
    else
    {
        P.rail = RGB(232, 232, 235);      P.side = RGB(238, 238, 240);      P.content = RGB(226, 226, 230);
        P.border1 = RGB(214, 214, 219);   P.border2 = RGB(218, 218, 222);   P.outer = RGB(205, 205, 210);
        P.card = RGB(247, 247, 248);      P.cardBorder = RGB(218, 218, 222); P.sep = RGB(230, 230, 233);
        P.sel = RGB(222, 226, 230);       P.combo = RGB(234, 235, 238);     P.comboHover = RGB(224, 226, 230);
        P.text = RGB(78, 78, 82);         P.textBright = RGB(18, 18, 20);   P.title = RGB(120, 120, 126);
        P.sideText = RGB(92, 95, 105);    P.railIcon = RGB(70, 72, 82);     P.crumb = RGB(140, 145, 155);
        P.hdrIcon = RGB(140, 140, 145);   P.placeholder = RGB(140, 140, 145); P.iconOff = RGB(165, 165, 170);
        P.offTrack = RGB(222, 223, 227);  P.knobOff = RGB(170, 170, 175);   P.sliderBg = RGB(220, 222, 228);
        P.knob = RGB(255, 255, 255);
        P.popup = RGB(250, 250, 251);     P.popupBorder = RGB(214, 214, 218); P.popupSep = RGB(232, 232, 236);
        P.keyBox = RGB(226, 228, 233);    P.keyText = RGB(60, 60, 64);
        P.win = RGB(240, 240, 242);       P.winBorder = RGB(210, 210, 214); P.searchBorder = RGB(222, 222, 226);
        P.backdropTop = RGB(70, 66, 70);  P.backdropBottom = RGB(30, 30, 34);
    }
}

// ---------------------------------------------------------------------------
// Localization
// ---------------------------------------------------------------------------
static int gLang = 0; // 0 = en, 1 = ru
static const char* T(const char* s)
{
    if (gLang != 1) return s;
    static std::unordered_map<std::string, const char*> ru = {
        {"Info Screen","Инфо экран"},{"Aggro Drawer","Агро линии"},{"Camera","Камера"},{"Heroes Overlay","Оверлей героев"},
        {"Info Overlay","Инфо оверлей"},{"Notifications","Уведомления"},{"Offscreen","Вне экрана"},{"Radius","Радиус"},
        {"Show Me More","Показать больше"},{"Visible Settings","Видимость"},{"Ward Helper","Помощник вардов"},
        {"Main","Главная"},{"Search","Поиск"},{"Items Overlay","Оверлей предметов"},{"Skills Overlay","Оверлей скиллов"},
        {"Modifiers Overlay","Оверлей модификаторов"},{"Bars Overlay","Оверлей полосок"},{"Extra Settings","Доп. настройки"},
        {"Enable","Включить"},{"Settings","Настройки"},{"Show On","Показывать на"},{"Align","Выравнивание"},
        {"Minified","Компактно"},{"Custom Bars","Свои полоски"},{"Experience","Опыт"},{"OnHover Radius","Радиус наведения"},
        {"Enemies","Враги"},{"Allies","Союзники"},{"Local","Свой"},{"Top","Сверху"},{"Bottom","Снизу"},{"Left","Слева"},
        {"Right","Справа"},{"Health","Здоровье"},{"Mana","Мана"},{"Theme Colors","Цвета темы"},{"Size","Размер"},
        {"Opacity","Прозрачность"},{"Hover Opacity","При наведении"},{"New Bind","Новый бинд"},{"Bind List","Список биндов"},
        {"Copy Lua Path","Копировать Lua путь"},{"Reset","Сбросить"},{"Key","Клавиша"},{"Type","Тип"},{"Value","Значение"},
        {"Toggle","Переключ."},{"Hold","Удержание"},{"Always","Всегда"},{"Menu Bind","Бинд меню"},{"Reload Bind","Бинд перезагрузки"},
        {"Log Window","Окно логов"},{"Binds Island","Остров биндов"},{"Enable Scripts","Скрипты"},{"Language","Язык"},
        {"Animation Duration","Длит. анимации"},{"Developer Mode","Режим разработчика"},{"Visual","Внешний вид"},
        {"Menu Blur Factor","Размытие меню"},{"Disable Shadows","Отключить тени"},{"Menu Scale","Масштаб меню"},
        {"Enable Visuals","Включить визуалы"},{"Theme","Тема"},{"Serverside","Серверные"},{"Clientside","Клиентские"},
        {"Developer","Разработка"},{"Log","Лог"},{"No binds yet","Биндов пока нет"},{"Nothing here yet","Здесь пока пусто"},
        {"Favorites","Избранное"},{"World","Мир"},{"Heroes","Герои"},{"Abilities","Способности"},{"Effects","Эффекты"},
        {"Utility","Утилиты"},{"Scripts","Скрипты"},{"Cloud","Облако"},{"This module has no options yet.","У этого модуля пока нет опций."},
        {"Copied","Скопировано"},{"Reset to default","Сброшено"},{"Press","Нажмите"},{"to open the menu","чтобы открыть меню"},
        {"Scripts reloaded","Скрипты перезагружены"},{"Custom color","Свой цвет"},{"Close","Закрыть"},{"Binds","Бинды"},
        {"Notifications are enabled","Уведомления включены"},{"Press a key... (Esc = none)","Нажмите клавишу... (Esc = нет)"},
    };
    auto it = ru.find(s);
    return it != ru.end() ? it->second : s;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
struct GearCfg { bool themeColors = true; float size = 29.f, opacity = 100.f, hoverOpacity = 60.f; };

struct State
{
    // Heroes overlay
    bool  itemsEnable = true;  int itemsShowOn = 1; int itemsAlign = 3;
    bool  skillsEnable = true; bool skillsMinified = false; int skillsShowOn = 3; int skillsAlign = 0;
    bool  modEnable = true;    int modShowOn = 5; int modAlign = 0;
    bool  barsEnable = true;   int barsCustom = 0; bool barsExp = false;
    float hoverRadius = 110.f;
    // settings
    int   menuKey = ImGuiKey_Insert, reloadKey = ImGuiKey_F7;
    bool  logWindow = false, bindsIsland = true;
    int   scripts = 3; int lang = 0; float animDur = 120.f; bool devMode = true;
    float blur = 0.30f; bool disableShadows = true; int scaleIdx = 4; bool enableVisuals = true;
    int   theme = 2; ImVec4 customAccent = ImVec4(0.94f, 0.23f, 0.23f, 1.f);
};
static State st;
static State stDefault;

struct Bind
{
    std::string path, label;
    bool* target = nullptr;
    int   key = ImGuiKey_None;
    int   type = 0;          // 0 toggle, 1 hold, 2 always
    bool  value = true;
    bool  visible = true;
};
static std::vector<Bind> gBinds;

struct CtxTarget
{
    std::string path, label;
    bool* b = nullptr; int* i = nullptr; float* f = nullptr;
    bool db = false; int di = 0; float df = 0.f;
};
static CtxTarget gCtx;
static bool      gCtxRequest = false;
static ImVec2    gCtxPos;
static int       gEditBind = -1;
static bool      gBindPanelRequest = false;
static ImVec2    gBindPanelPos;
static ImVec2    gColorPickPos;

static GearCfg*  gGearCfg = nullptr;
static std::string gGearTitle;
static bool      gGearRequest = false;
static ImVec2    gGearAnchor;
static bool      gBindListRequest = false;
static bool      gColorPickRequest = false;

static int*      gListenKey = nullptr;     // key box waiting for input
static std::unordered_map<std::string, GearCfg> gGears;

static bool  gVisible = true;
static bool  gSettingsOpen = true;
static int   gRail = 3;                     // selected rail category (Info Screen)
static int   gPage = 2;                     // Heroes Overlay
static char  gSearch[64] = "";
static float S = 1.f;
static ImVec2 gMenuPos(40, 40), gSettingsPos(900, 40);
static bool  gPosSet = false;

struct Toast { std::string text; float t; };
static std::vector<Toast> gToasts;
static std::vector<std::string> gLog;

static void Log(const std::string& s)
{
    char buf[32]; snprintf(buf, sizeof(buf), "[%7.2f] ", ImGui::GetTime());
    gLog.push_back(buf + s);
    if (gLog.size() > 200) gLog.erase(gLog.begin());
}
static void Notify(const std::string& s) { gToasts.push_back({ s, 0.f }); Log(s); }

static const int kScales[5] = { 100, 125, 150, 175, 200 };

// ---------------------------------------------------------------------------
// Fonts & drawing helpers
// ---------------------------------------------------------------------------
static ImFont* FText = nullptr;
static ImFont* FBold = nullptr;
static ImFont* FIcon = nullptr;
static const float kText = 18.6f;   // body text size (design units)
static const float kIcon = 15.f;   // icon size (design units)

static std::unordered_map<ImGuiID, float> gAnim;
static float Anim(ImGuiID id, float target, float speed = 14.f)
{
    auto it = gAnim.find(id);
    if (it == gAnim.end()) { gAnim[id] = target; return target; }
    float dur = ImMax(10.f, st.animDur) / 100.f;
    float k = 1.f - expf(-speed * ImGui::GetIO().DeltaTime / dur);
    it->second += (target - it->second) * k;
    if (fabsf(target - it->second) < 0.002f) it->second = target;
    return it->second;
}

static ImVec2 O; // current origin (window pos)
static inline ImVec2 Pt(float x, float y) { return ImVec2(O.x + x * S, O.y + y * S); }
static inline float  U(float v) { return v * S; }

static void FadeVerts(ImDrawList* dl, int v0, float x0, float x1)
{
    for (int i = v0; i < dl->VtxBuffer.Size; i++)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        float t = ImSaturate((v.pos.x - x0) / ImMax(1.f, x1 - x0));
        v.col = WithA(v.col, 1.f - t);
    }
}

static float TextW(const char* t, float size = kText, ImFont* f = nullptr)
{
    if (!f) f = FText;
    return f->CalcTextSizeA(U(size), FLT_MAX, 0.f, t).x;
}

// Text vertically centred on cy, left aligned at x. Optional clip + fade at the right.
static void TextL(ImDrawList* dl, float x, float cy, ImU32 col, const char* t, float size = kText, float clip_x = FLT_MAX, float fade = 0.f, ImFont* f = nullptr)
{
    if (!f) f = FText;
    float h = U(size);
    ImVec2 p(IM_ROUND(x), IM_ROUND(cy - h * 0.5f - h * 0.035f));
    int v0 = dl->VtxBuffer.Size;
    bool clip = clip_x < FLT_MAX;
    if (clip) dl->PushClipRect(ImVec2(x - 4, cy - h), ImVec2(clip_x, cy + h), true);
    dl->AddText(f, h, p, col, t);
    if (clip) { dl->PopClipRect(); if (fade > 0.f) FadeVerts(dl, v0, clip_x - fade, clip_x); }
}
static void TextR(ImDrawList* dl, float right, float cy, ImU32 col, const char* t, float size = kText)
{
    TextL(dl, right - TextW(t, size), cy, col, t, size);
}

static void Icon(ImDrawList* dl, const char* ic, float size, ImVec2 c, ImU32 col, ImFont* f = nullptr)
{
    if (!f) f = FIcon;
    unsigned int cp = 0; ImTextCharFromUtf8(&cp, ic, nullptr);
    float sz = U(size);
    ImFontBaked* b = f->GetFontBaked(sz);
    const ImFontGlyph* g = b ? b->FindGlyphNoFallback((ImWchar)cp) : nullptr;
    ImVec2 off = g ? ImVec2((g->X0 + g->X1) * 0.5f, (g->Y0 + g->Y1) * 0.5f) : ImVec2(sz * 0.5f, sz * 0.5f);
    dl->AddText(f, sz, ImVec2(IM_ROUND(c.x - off.x), IM_ROUND(c.y - off.y)), col, ic);
}

// Custom drawn pseudo icons: "@ooo" "@exp" "@0110" "@curly" "@rays" "@folder" "@newbind"
static void DrawIcon(ImDrawList* dl, const char* ic, ImVec2 c, ImU32 col, float size = kIcon)
{
    if (!ic) return;
    if (ic[0] != '@') { Icon(dl, ic, size, c, col); return; }
    float th = ImMax(1.f, U(1.1f));
    if (!strcmp(ic, "@ooo"))
    {
        for (int i = -1; i <= 1; i++) dl->AddCircle(ImVec2(c.x + i * U(4.6f), c.y), U(1.7f), col, 12, th);
    }
    else if (!strcmp(ic, "@exp"))
    {
        dl->AddLine(ImVec2(c.x - U(7.5f), c.y), ImVec2(c.x - U(1.5f), c.y), col, U(1.6f));
        dl->AddLine(ImVec2(c.x + U(3.5f), c.y), ImVec2(c.x + U(7.5f), c.y), col, U(1.6f));
        dl->AddRectFilled(ImVec2(c.x - U(2.f), c.y - U(3.5f)), ImVec2(c.x + U(3.f), c.y + U(3.5f)), col, U(1.f));
        dl->AddRectFilled(ImVec2(c.x - U(0.5f), c.y - U(1.5f)), ImVec2(c.x + U(1.5f), c.y + U(1.5f)), P.card);
    }
    else if (!strcmp(ic, "@0110"))
    {
        float fs = 12.5f;
        for (int k = 0; k < 2; k++)
        {
            float ox = k * U(0.45f);
            TextL(dl, c.x - TextW("01", fs, FBold) * 0.5f + ox, c.y - U(4.6f), col, "01", fs, FLT_MAX, 0, FBold);
            TextL(dl, c.x - TextW("10", fs, FBold) * 0.5f + ox, c.y + U(4.9f), col, "10", fs, FLT_MAX, 0, FBold);
        }
    }
    else if (!strcmp(ic, "@curly"))
    {
        float fs = 20.f;
        for (int k = 0; k < 2; k++)
            TextL(dl, c.x - TextW("{ }", fs, FBold) * 0.5f + k * U(0.5f), c.y - U(0.5f), col, "{ }", fs, FLT_MAX, 0, FBold);
    }
    else if (!strcmp(ic, "@rays"))
    {
        float t = U(1.5f);
        dl->AddLine(ImVec2(c.x - U(8.5f), c.y + U(4.f)), ImVec2(c.x + U(8.5f), c.y + U(4.f)), col, t);
        for (int i = 0; i < 5; i++)
        {
            float a = IM_PI + IM_PI * (i + 0.5f) / 5.f;
            ImVec2 d(cosf(a), sinf(a));
            ImVec2 base(c.x, c.y + U(4.f));
            dl->AddLine(ImVec2(base.x + d.x * U(4.5f), base.y + d.y * U(4.5f)), ImVec2(base.x + d.x * U(8.f), base.y + d.y * U(8.f)), col, t);
        }
        dl->AddCircleFilled(ImVec2(c.x - U(4.f), c.y + U(1.2f)), U(1.f), col);
        dl->AddCircleFilled(ImVec2(c.x + U(4.f), c.y + U(1.2f)), U(1.f), col);
    }
    else if (!strcmp(ic, "@newbind"))
    {
        Icon(dl, ICON_FA_LAYER_GROUP, size, ImVec2(c.x - U(1.5f), c.y), col);
        ImVec2 pc(c.x + U(5.f), c.y + U(4.f));
        dl->AddCircleFilled(pc, U(5.f), P.popup);
        dl->AddCircle(pc, U(3.8f), col, 16, U(1.3f));
        dl->AddLine(ImVec2(pc.x - U(2.f), pc.y), ImVec2(pc.x + U(2.f), pc.y), col, U(1.2f));
        dl->AddLine(ImVec2(pc.x, pc.y - U(2.f)), ImVec2(pc.x, pc.y + U(2.f)), col, U(1.2f));
    }
    else if (!strcmp(ic, "@drop"))
    {
        Icon(dl, ICON_FA_DROPLET, size, ImVec2(c.x - U(1.5f), c.y + U(0.5f)), col);
        dl->AddCircle(ImVec2(c.x + U(5.5f), c.y - U(5.f)), U(1.7f), col, 12, U(1.2f));
    }
    else if (!strcmp(ic, "@folder"))
    {
        Icon(dl, ICON_FA_FOLDER, size, c, col);
        Icon(dl, ICON_FA_GEAR, size * 0.5f, ImVec2(c.x, c.y + U(1.f)), P.win);
    }
}

static void Shadow(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, float strength = 1.f)
{
    if (st.disableShadows) return;
    dl->PushClipRectFullScreen();
    for (int i = 0; i < 12; i++)
    {
        float e = (i + 1) * U(1.5f);
        dl->AddRectFilled(ImVec2(a.x - e, a.y - e + U(4)), ImVec2(b.x + e, b.y + e + U(4)), IM_COL32(0, 0, 0, (int)(9 * strength)), r + e);
    }
    dl->PopClipRect();
}

// Invisible button at absolute rect
static bool Btn(const char* id, ImVec2 a, ImVec2 b, bool overlap = false, ImGuiButtonFlags flags = 0)
{
    ImGui::SetCursorScreenPos(a);
    if (overlap) ImGui::SetNextItemAllowOverlap();
    return ImGui::InvisibleButton(id, ImVec2(ImMax(1.f, b.x - a.x), ImMax(1.f, b.y - a.y)), flags);
}

static void Chevron(ImDrawList* dl, ImVec2 c, ImU32 col)
{
    ImVec2 pts[3] = { ImVec2(c.x - U(5.3f), c.y - U(2.6f)), ImVec2(c.x, c.y + U(2.6f)), ImVec2(c.x + U(5.3f), c.y - U(2.6f)) };
    dl->AddPolyline(pts, 3, col, 0, U(1.8f));
}

static void Toggle(ImDrawList* dl, ImGuiID id, float right, float cy, bool v)
{
    float t = Anim(id, v ? 1.f : 0.f, 16.f);
    ImVec2 a(right - U(30), cy - U(8)), b(right, cy + U(8));
    // track
    dl->AddRectFilled(a, b, LerpC(P.offTrack, LerpC(P.card, P.accent, 0.17f), t), U(8));
    // knob
    float kx = ImLerp(a.x + U(8), b.x - U(8), t);
    if (t > 0.01f)
    {
        dl->PushClipRect(a, b, true);
        for (int i = 1; i <= 4; i++) dl->AddCircleFilled(ImVec2(kx - U(0.9f) * i, cy), U(8), IM_COL32(0, 0, 0, (int)(14 * t)), 32);
        dl->PopClipRect();
    }
    dl->AddCircleFilled(ImVec2(kx, cy), U(8), LerpC(P.knobOff, P.accent, t), 32);
}

static std::string Lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static std::string PathOf(const char* a, const char* b)
{
    std::string s = Lower(std::string("info_screen.heroes_overlay.") + a + "." + b);
    for (auto& c : s) if (c == ' ') c = '_';
    return s;
}

static const char* KeyName(int k)
{
    switch (k)
    {
    case ImGuiKey_None: return "NONE";
    case ImGuiKey_Insert: return "INS";      case ImGuiKey_Delete: return "DEL";
    case ImGuiKey_Home: return "HOME";       case ImGuiKey_End: return "END";
    case ImGuiKey_PageUp: return "PGUP";     case ImGuiKey_PageDown: return "PGDN";
    case ImGuiKey_LeftShift: return "LSHIFT";case ImGuiKey_RightShift: return "RSHIFT";
    case ImGuiKey_LeftCtrl: return "LCTRL";  case ImGuiKey_RightCtrl: return "RCTRL";
    case ImGuiKey_LeftAlt: return "LALT";    case ImGuiKey_RightAlt: return "RALT";
    case ImGuiKey_Space: return "SPACE";     case ImGuiKey_Escape: return "ESC";
    case ImGuiKey_Enter: return "ENTER";     case ImGuiKey_Tab: return "TAB";
    case ImGuiKey_Backspace: return "BKSP";  case ImGuiKey_CapsLock: return "CAPS";
    case ImGuiKey_MouseMiddle: return "MMB"; case ImGuiKey_MouseX1: return "MB4"; case ImGuiKey_MouseX2: return "MB5";
    default: break;
    }
    const char* n = ImGui::GetKeyName((ImGuiKey)k);
    return n && n[0] ? n : "?";
}

// ---------------------------------------------------------------------------
// Popup helper (custom drawn, fade + slide in)
// ---------------------------------------------------------------------------
struct PopupFrame { ImDrawList* dl; int v0; float fade; };
static std::vector<PopupFrame> gPopupStack;

static bool PopupBegin(const char* id, ImVec2 pos, ImVec2 size, float rounding = 5.f)
{
    ImGuiID pid = ImGui::GetID(id);
    if (!ImGui::IsPopupOpen(pid, 0)) { gAnim[pid ^ 0x5bd1e995] = 0.f; return false; }
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    pos.x = ImClamp(pos.x, 4.f, ImMax(4.f, ds.x - size.x - 4.f));
    pos.y = ImClamp(pos.y, 4.f, ImMax(4.f, ds.y - size.y - 4.f));
    ImGui::SetNextWindowPos(ImFloor(pos));
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
    bool open = ImGui::BeginPopup(id, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
    if (!open) return false;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float fade = Anim(pid ^ 0x5bd1e995, 1.f, 20.f);
    gPopupStack.push_back({ dl, dl->VtxBuffer.Size, fade });
    ImVec2 a = ImGui::GetWindowPos(), b(a.x + size.x, a.y + size.y);
    Shadow(dl, a, b, U(rounding));
    dl->AddRectFilled(a, b, P.popup, U(rounding));
    dl->AddRect(a, b, P.popupBorder, U(rounding), 0, 1.f);
    return true;
}
static void PopupEnd()
{
    PopupFrame f = gPopupStack.back(); gPopupStack.pop_back();
    if (f.fade < 1.f)
    {
        float dy = (1.f - f.fade) * U(5.f);
        for (int i = f.v0; i < f.dl->VtxBuffer.Size; i++)
        {
            ImDrawVert& v = f.dl->VtxBuffer[i];
            v.col = WithA(v.col, f.fade);
            v.pos.y -= dy;
        }
    }
    ImGui::EndPopup();
}

// ---------------------------------------------------------------------------
// Card + rows
// ---------------------------------------------------------------------------
struct CardCtx
{
    ImDrawList* dl; float x, y, w, cur, pad; bool first; bool bg; const char* title;
};
static CardCtx C;

static void CardBegin(const char* title, ImVec2 pos, float w, bool bg = true, float pad = 12.f, float title_cy = 18.5f, float first_row = 32.5f)
{
    C.dl = ImGui::GetWindowDrawList();
    C.x = pos.x; C.y = pos.y; C.w = w; C.pad = U(pad); C.first = true; C.bg = bg; C.title = title;
    C.cur = pos.y + U(first_row);
    ImGui::PushID(title ? title : "##card");
    if (bg) { C.dl->ChannelsSplit(2); C.dl->ChannelsSetCurrent(1); }
    if (title) TextL(C.dl, C.x + C.pad, C.y + U(title_cy), P.title, T(title));
}
static float CardEnd()
{
    float h = C.cur - C.y;
    ImGui::PopID();
    if (C.bg)
    {
        C.dl->ChannelsSetCurrent(0);
        ImVec2 a(C.x, C.y), b(C.x + C.w, C.cur);
        C.dl->AddRectFilled(a, b, P.card, U(5));
        C.dl->AddRect(a, b, P.cardBorder, U(5), 0, 1.f);
        C.dl->ChannelsMerge();
    }
    return h;
}

struct Row { ImVec2 a, b; float cy, left, right, iconX, labelX; };
static Row RowBegin(float h, bool has_icon = true)
{
    Row r;
    if (!C.first)
        C.dl->AddLine(ImVec2(C.x + C.pad, C.cur), ImVec2(C.x + C.w - C.pad, C.cur), C.bg ? P.sep : P.popupSep, 1.f);
    C.first = false;
    r.a = ImVec2(C.x, C.cur); r.b = ImVec2(C.x + C.w, C.cur + U(h));
    r.cy = C.cur + U(h) * 0.5f;
    r.left = C.x + C.pad; r.right = C.x + C.w - C.pad;
    r.iconX = r.left + U(10);
    r.labelX = has_icon ? r.left + U(28) : r.left;
    C.cur += U(h);
    return r;
}

static void OpenContext(const char* label, bool* b, int* i, float* f, bool db, int di, float df)
{
    gCtx = CtxTarget();
    gCtx.path = PathOf(C.title ? C.title : "settings", label);
    gCtx.label = std::string(C.title ? C.title : "Settings") + " / " + label;
    gCtx.b = b; gCtx.i = i; gCtx.f = f; gCtx.db = db; gCtx.di = di; gCtx.df = df;
    gCtxRequest = true;
    gCtxPos = ImGui::GetIO().MousePos;
}
static bool RowRightClicked(const Row& r)
{
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsMouseHoveringRect(r.a, r.b);
}

static bool GearButton(const char* id, ImVec2 c, GearCfg* cfg, const char* title)
{
    ImVec2 a(c.x - U(10), c.y - U(10)), b(c.x + U(10), c.y + U(10));
    bool pressed = Btn(id, a, b);
    bool hov = ImGui::IsItemHovered();
    float t = Anim(ImGui::GetItemID(), hov ? 1.f : 0.f);
    Icon(C.dl, ICON_FA_GEAR, kIcon, c, LerpC(P.text, P.textBright, t));
    if (pressed && cfg)
    {
        gGearCfg = cfg; gGearTitle = title; gGearRequest = true; gGearAnchor = c;
    }
    return pressed;
}

static GearCfg* Gear(const char* key) { return &gGears[key]; }

// Toggle row. icon may be nullptr / "@..." / FA glyph. Returns true when toggled.
static bool RowToggle(const char* icon, const char* label, bool* v, GearCfg* gear = nullptr, float h = 38.f)
{
    ImGui::PushID(label);
    Row r = RowBegin(h, icon != nullptr);
    bool def = *v;
    { auto it = gAnim.find(ImGui::GetID("##def")); if (it == gAnim.end()) gAnim[ImGui::GetID("##def")] = def ? 1.f : 0.f; else def = it->second > 0.5f; }
    bool pressed = Btn("##row", r.a, r.b, true);
    bool hov = ImGui::IsItemHovered();
    if (pressed) { *v = !*v; Log(std::string(label) + (*v ? " -> on" : " -> off")); }
    float t = Anim(ImGui::GetID("##lbl"), *v ? 1.f : 0.f, 16.f);
    ImU32 tc = LerpC(P.text, P.textBright, t);
    if (hov && !*v) tc = LerpC(tc, P.textBright, 0.25f);
    DrawIcon(C.dl, icon, ImVec2(r.iconX, r.cy), LerpC(P.iconOff, P.accent, t));
    float tr = r.right - U(30);
    if (gear) tr -= U(25);
    TextL(C.dl, r.labelX, r.cy, tc, T(label), kText, tr - U(4), U(12));
    Toggle(C.dl, ImGui::GetID("##tg"), r.right, r.cy, *v);
    if (gear) GearButton("##gear", ImVec2(r.right - U(30) - U(12.5f), r.cy), gear, C.title ? C.title : label);
    if (RowRightClicked(r)) OpenContext(label, v, nullptr, nullptr, def, 0, 0);
    ImGui::PopID();
    return pressed;
}

// Combo dropdown popup (centered vertically on the combo, same width)
static bool ComboPopup(const char* id, ImVec2 a, ImVec2 b, const char* const* items, int n, int* val, bool multi)
{
    bool changed = false;
    float ih = U(25), pad = U(2.5f);
    float h = ih * n + pad * 2;
    float w = b.x - a.x;
    float cy = (a.y + b.y) * 0.5f;
    ImVec2 pos(a.x, cy - h * 0.5f);
    if (n == 1) pos.y = a.y;
    if (PopupBegin(id, pos, ImVec2(w, h), 4.f))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        for (int i = 0; i < n; i++)
        {
            ImGui::PushID(i);
            ImVec2 ia(wp.x + U(3), wp.y + pad + ih * i), ib(wp.x + w - U(3), wp.y + pad + ih * (i + 1));
            bool pressed = Btn("##it", ia, ib);
            bool hov = ImGui::IsItemHovered();
            float ht = Anim(ImGui::GetItemID(), hov ? 1.f : 0.f, 20.f);
            if (ht > 0.01f) dl->AddRectFilled(ia, ib, WithA(P.comboHover, ht), U(3));
            bool sel = multi ? ((*val >> i) & 1) != 0 : *val == i;
            float icy = (ia.y + ib.y) * 0.5f;
            if (sel)
            {
                Icon(dl, ICON_FA_CHECK, 14.f, ImVec2(wp.x + U(17.5f), icy), P.accent);
                TextL(dl, wp.x + U(30.5f), icy, P.accent, T(items[i]), kText, wp.x + w - U(6), U(8));
            }
            else
                TextL(dl, wp.x + U(12), icy, LerpC(P.text, P.textBright, 0.7f), T(items[i]), kText, wp.x + w - U(6), U(8));
            if (pressed)
            {
                changed = true;
                if (multi) *val ^= (1 << i);
                else { *val = i; ImGui::CloseCurrentPopup(); }
            }
            ImGui::PopID();
        }
        PopupEnd();
    }
    return changed;
}

static std::string ComboText(const char* const* items, int n, int val, bool multi)
{
    if (!multi) return (val >= 0 && val < n) ? T(items[val]) : "";
    std::string s;
    for (int i = 0; i < n; i++) if ((val >> i) & 1) { if (!s.empty()) s += ", "; s += T(items[i]); }
    if (s.empty()) s = "-";
    return s;
}

static bool ComboBox(const char* id, ImVec2 a, ImVec2 b, const char* const* items, int n, int* val, bool multi)
{
    ImGui::PushID(id);
    bool pressed = Btn("##cb", a, b);
    bool hov = ImGui::IsItemHovered();
    float t = Anim(ImGui::GetItemID(), hov ? 1.f : 0.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, LerpC(P.combo, P.comboHover, t), U(4));
    float cy = (a.y + b.y) * 0.5f;
    std::string txt = ComboText(items, n, *val, multi);
    float chevX = b.x - U(15);
    TextL(dl, a.x + U(8.5f), cy, LerpC(P.text, P.textBright, t * 0.4f), txt.c_str(), kText, chevX - U(8), U(11));
    Chevron(dl, ImVec2(chevX, cy + U(0.3f)), LerpC(P.text, P.textBright, t * 0.4f));
    if (pressed) ImGui::OpenPopup("##drop");
    bool changed = ComboPopup("##drop", a, b, items, n, val, multi);
    ImGui::PopID();
    return changed;
}

static bool RowCombo(const char* icon, const char* label, int* val, const char* const* items, int n, bool multi, GearCfg* gear = nullptr, float h = 44.f)
{
    ImGui::PushID(label);
    Row r = RowBegin(h, icon != nullptr);
    int def = *val;
    { ImGuiID did = ImGui::GetID("##def"); auto it = gAnim.find(did); if (it == gAnim.end()) gAnim[did] = (float)def; else def = (int)it->second; }
    Btn("##row", r.a, r.b, true);
    DrawIcon(C.dl, icon, ImVec2(r.iconX, r.cy), P.accent);
    ImVec2 ca(r.right - U(120), r.cy - U(12)), cb(r.right, r.cy + U(12));
    float tr = ca.x - (gear ? U(25) : 0.f);
    TextL(C.dl, r.labelX, r.cy, P.text, T(label), kText, tr - U(4), U(12));
    if (gear) GearButton("##gear", ImVec2(ca.x - U(12.5f), r.cy), gear, C.title ? C.title : label);
    bool changed = ComboBox("##combo", ca, cb, items, n, val, multi);
    if (RowRightClicked(r)) OpenContext(label, nullptr, val, nullptr, false, def, 0);
    ImGui::PopID();
    return changed;
}

static bool RowSlider(const char* icon, const char* label, float* v, float mn, float mx, const char* fmt, float h = 54.5f, float label_off = 18.f, float track_off = 39.f)
{
    ImGui::PushID(label);
    Row r = RowBegin(h, icon != nullptr);
    float def = *v;
    { ImGuiID did = ImGui::GetID("##def"); auto it = gAnim.find(did); if (it == gAnim.end()) gAnim[did] = def; else def = it->second; }
    float ly = r.a.y + U(label_off), ty = r.a.y + U(track_off);
    DrawIcon(C.dl, icon, ImVec2(r.iconX, ly), P.accent);
    char buf[32]; snprintf(buf, sizeof(buf), fmt, *v);
    float vw = TextW(buf);
    TextL(C.dl, r.labelX, ly, P.text, T(label), kText, r.right - vw - U(8), U(12));
    TextR(C.dl, r.right, ly, P.text, buf);
    // track
    ImVec2 ta(r.left, ty - U(2.5f)), tb(r.right, ty + U(2.5f));
    Btn("##sl", ImVec2(ta.x - U(4), ty - U(9)), ImVec2(tb.x + U(4), ty + U(9)));
    bool active = ImGui::IsItemActive(), hov = ImGui::IsItemHovered();
    if (active)
    {
        float t = ImSaturate((ImGui::GetIO().MousePos.x - ta.x) / (tb.x - ta.x));
        float nv = mn + (mx - mn) * t;
        if (strstr(fmt, "%.0f")) nv = roundf(nv);
        *v = nv;
    }
    float t = ImSaturate((*v - mn) / (mx - mn));
    float kx = ImLerp(ta.x + U(6), tb.x - U(6), t);
    C.dl->AddRectFilled(ta, tb, P.sliderBg, U(2.5f));
    C.dl->AddRectFilled(ta, ImVec2(kx, tb.y), P.accent, U(2.5f));
    float ka = Anim(ImGui::GetID("##k"), (active || hov) ? 1.f : 0.f);
    if (!st.disableShadows) C.dl->AddCircleFilled(ImVec2(kx, ty + U(0.8f)), U(7.5f), IM_COL32(0, 0, 0, 70), 32);
    C.dl->AddCircleFilled(ImVec2(kx, ty), U(6.f + ka * 0.8f), P.knob, 32);
    if (RowRightClicked(r)) OpenContext(label, nullptr, nullptr, v, false, 0, def);
    ImGui::PopID();
    return active;
}

static void KeyBox(const char* id, ImVec2 rc /*right-center*/, int* key, float w = 35.f, float h = 16.f)
{
    ImGui::PushID(id);
    bool listening = gListenKey == key;
    const char* name = KeyName(*key);
    float bw = listening ? U(w) : ImMax(U(w), TextW(name, 16.f) + U(12));
    ImVec2 a(rc.x - bw, rc.y - U(h) * 0.5f), b(rc.x, rc.y + U(h) * 0.5f);
    bool pressed = Btn("##key", a, b);
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, hov ? LerpC(P.keyBox, P.textBright, 0.06f) : P.keyBox, U(2));
    if (listening)
    {
        float tt = (float)ImGui::GetTime();
        for (int i = -1; i <= 1; i++)
        {
            float al = 0.55f + 0.45f * sinf(tt * 6.f - i * 0.9f);
            dl->AddCircleFilled(ImVec2(rc.x - bw * 0.5f + i * U(6.5f), rc.y), U(1.9f), WithA(RGB(119, 120, 123), al));
        }
        if (hov) ImGui::SetTooltip("%s", T("Press a key... (Esc = none)"));
    }
    else
        TextL(dl, a.x + (bw - TextW(name, 16.f)) * 0.5f, rc.y, P.keyText, name, 16.f);
    if (pressed) gListenKey = listening ? nullptr : key;
    ImGui::PopID();
}

static void RowKey(const char* icon, const char* label, int* key)
{
    ImGui::PushID(label);
    Row r = RowBegin(38.f, icon != nullptr);
    Btn("##row", r.a, r.b, true);
    DrawIcon(C.dl, icon, ImVec2(r.iconX, r.cy), P.accent);
    TextL(C.dl, r.labelX, r.cy, P.text, T(label), kText, r.right - U(44), U(12));
    KeyBox("##kb", ImVec2(r.right, r.cy), key);
    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Option lists
// ---------------------------------------------------------------------------
static const char* kShowOn[] = { "Enemies", "Allies", "Local" };
static const char* kAlign[] = { "Top", "Bottom", "Left", "Right" };
static const char* kBars[] = { "Health", "Mana" };
static const char* kBindType[] = { "Toggle", "Hold", "Always" };
static const char* kScripts[] = { "Serverside", "Clientside", "Developer" };
static const char* kLang[] = { "en", "ru" };
static const char* kScaleNames[] = { "100", "125", "150", "175", "200" };

struct SideItem { const char* icon; const char* name; };
static SideItem kSide[] = {
    { ICON_FA_JET_FIGHTER, "Aggro Drawer" }, { ICON_FA_BINOCULARS, "Camera" }, { ICON_FA_LIST_UL, "Heroes Overlay" },
    { ICON_FA_TABLE_COLUMNS, "Info Overlay" }, { ICON_FA_BELL, "Notifications" }, { ICON_FA_CIRCLE_UP, "Offscreen" },
    { ICON_FA_CIRCLE_CHEVRON_DOWN, "Radius" }, { ICON_FA_EYE_SLASH, "Show Me More" }, { ICON_FA_EYE, "Visible Settings" },
    { ICON_FA_HURRICANE, "Ward Helper" },
};
struct RailItem { const char* icon; const char* name; };
static RailItem kRail[] = {
    { ICON_FA_STAR, "Favorites" }, { ICON_FA_EARTH_AMERICAS, "World" }, { ICON_FA_USER_GROUP, "Heroes" },
    { ICON_FA_IMAGE, "Info Screen" }, { ICON_FA_DRAGON, "Abilities" }, { ICON_FA_WAND_MAGIC_SPARKLES, "Effects" },
    { ICON_FA_SCREWDRIVER_WRENCH, "Utility" }, { ICON_FA_CODE, "Scripts" }, { ICON_FA_CLOUD, "Cloud" },
};

// ---------------------------------------------------------------------------
// Global popups: context menu, bind panel, gear popup, bind list
// ---------------------------------------------------------------------------
static void BindPanel()
{
    if (gBindPanelRequest) { ImGui::OpenPopup("##bindpanel"); gBindPanelRequest = false; }
    if (gEditBind < 0 || gEditBind >= (int)gBinds.size()) return;
    ImVec2 pos = gBindPanelPos;
    ImVec2 size(U(311.5f), U(171.f));
    if (PopupBegin("##bindpanel", pos, size, 5.f))
    {
        Bind& bd = gBinds[gEditBind];
        ImVec2 wp = ImGui::GetWindowPos();
        CardBegin(nullptr, wp, size.x, false, 20.f, 0.f, 8.f);
        { Row r = RowBegin(41.f, false); TextL(C.dl, r.labelX, r.cy, P.text, T("Key")); KeyBox("##bk", ImVec2(r.right, r.cy), &bd.key, 35.f, 17.f); }
        {
            Row r = RowBegin(41.f, false); TextL(C.dl, r.labelX, r.cy, P.text, T("Type"));
            ComboBox("##bt", ImVec2(r.right - U(120), r.cy - U(11.5f)), ImVec2(r.right, r.cy + U(11.5f)), kBindType, 3, &bd.type, false);
        }
        {
            Row r = RowBegin(41.f, false);
            Btn("##bv", r.a, r.b);
            if (ImGui::IsItemClicked()) bd.value = !bd.value;
            float t = Anim(ImGui::GetID("bvl"), bd.value ? 1.f : 0.f);
            TextL(C.dl, r.labelX, r.cy, LerpC(P.text, P.textBright, t), T("Value"));
            Toggle(C.dl, ImGui::GetID("##bvt"), r.right, r.cy, bd.value);
        }
        {
            Row r = RowBegin(41.f, false);
            const char* icons[3] = { ICON_FA_TRASH, ICON_FA_EYE, ICON_FA_LIST_UL };
            float xs[3] = { r.left + U(13), (r.left + r.right) * 0.5f, r.right - U(11) };
            for (int i = 0; i < 3; i++)
            {
                ImGui::PushID(i);
                ImVec2 c(xs[i], r.cy);
                bool p = Btn("##f", ImVec2(c.x - U(14), c.y - U(14)), ImVec2(c.x + U(14), c.y + U(14)));
                bool hov = ImGui::IsItemHovered();
                ImU32 col = LerpC(P.text, P.textBright, hov ? 0.6f : 0.f);
                if (i == 1 && !bd.visible) col = P.iconOff;
                Icon(C.dl, (i == 1 && !bd.visible) ? ICON_FA_EYE_SLASH : icons[i], kIcon, c, col);
                if (p)
                {
                    if (i == 0) { gBinds.erase(gBinds.begin() + gEditBind); gEditBind = -1; if (gListenKey) gListenKey = nullptr; ImGui::CloseCurrentPopup(); }
                    else if (i == 1) bd.visible = !bd.visible;
                    else { gBindListRequest = true; ImGui::CloseCurrentPopup(); }
                }
                ImGui::PopID();
                if (gEditBind < 0) break;
            }
        }
        CardEnd();
        PopupEnd();
    }
}

static void ContextMenu()
{
    if (gCtxRequest) { ImGui::OpenPopup("##ctx"); gCtxRequest = false; gEditBind = -1; }
    int bindIdx[16]; int nb = 0;
    for (int i = 0; i < (int)gBinds.size() && nb < 16; i++) if (gBinds[i].path == gCtx.path) bindIdx[nb++] = i;
    int nItems = nb + 4;
    float ih = U(31);
    ImVec2 size(U(159), ih * nItems + U(3.5f) + U(7.5f));
    if (PopupBegin("##ctx", gCtxPos, size, 5.f))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float y = wp.y + U(3.5f);
        bool panelOpen = ImGui::IsPopupOpen("##bindpanel");
        for (int k = 0; k < nItems; k++)
        {
            ImGui::PushID(k);
            bool isBind = k < nb;
            int act = isBind ? -1 : k - nb; // 0 new, 1 list, 2 copy, 3 reset
            if (act == 3) dl->AddLine(ImVec2(wp.x + U(6), y), ImVec2(wp.x + size.x - U(6), y), P.popupSep, 1.f);
            ImVec2 ia(wp.x + U(6), y + U(4)), ib(wp.x + size.x - U(6), y + ih - U(4));
            bool pressed = Btn("##ci", ImVec2(wp.x, y), ImVec2(wp.x + size.x, y + ih));
            bool hov = ImGui::IsItemHovered();
            bool editing = isBind && gEditBind == bindIdx[k] && panelOpen;
            float ht = Anim(ImGui::GetItemID(), (hov || editing) ? 1.f : 0.f, 20.f);
            ImU32 hl = isBind ? LerpC(P.popup, P.accent, 0.22f) : LerpC(P.popup, P.textBright, 0.05f);
            if (ht > 0.01f) dl->AddRectFilled(ia, ib, WithA(hl, ht), U(4));
            float cy = y + ih * 0.5f;
            ImU32 col = act == 3 ? P.accent : LerpC(P.text, P.textBright, ht * 0.5f);
            const char* icon = nullptr; std::string label;
            if (isBind)
            {
                Bind& bd = gBinds[bindIdx[k]];
                icon = (bd.key == ImGuiKey_None || gListenKey == &bd.key) ? "@rays" : ICON_FA_KEYBOARD;
                label = bd.key == ImGuiKey_None ? T("New Bind") : KeyName(bd.key);
                if (bd.key != ImGuiKey_None) label += std::string("  ·  ") + T(kBindType[bd.type]);
            }
            else if (act == 0) { icon = "@newbind"; label = T("New Bind"); }
            else if (act == 1) { icon = ICON_FA_LIST_UL; label = T("Bind List"); }
            else if (act == 2) { icon = ICON_FA_COPY; label = T("Copy Lua Path"); }
            else { icon = ICON_FA_TRASH_ARROW_UP; label = T("Reset"); }
            DrawIcon(dl, icon, ImVec2(wp.x + U(25), cy), col);
            TextL(dl, wp.x + U(42), cy, col, label.c_str(), kText, wp.x + size.x - U(8), U(10));
            bool openPanel = false;
            if (isBind && (pressed || (hov && gEditBind != bindIdx[k]))) { gEditBind = bindIdx[k]; openPanel = true; }
            if (pressed && act == 0)
            {
                Bind nbd; nbd.path = gCtx.path; nbd.label = gCtx.label; nbd.target = gCtx.b;
                gBinds.push_back(nbd);
                gEditBind = (int)gBinds.size() - 1;
                gListenKey = &gBinds.back().key;
                openPanel = true;
                Log("New bind: " + gCtx.path);
            }
            if (pressed && act == 1) { gBindListRequest = true; ImGui::CloseCurrentPopup(); }
            if (pressed && act == 2) { ImGui::SetClipboardText(gCtx.path.c_str()); Notify(std::string(T("Copied")) + ": " + gCtx.path); ImGui::CloseCurrentPopup(); }
            if (pressed && act == 3)
            {
                if (gCtx.b) *gCtx.b = gCtx.db;
                if (gCtx.i) *gCtx.i = gCtx.di;
                if (gCtx.f) *gCtx.f = gCtx.df;
                Notify(std::string(T("Reset to default")) + ": " + gCtx.label);
                ImGui::CloseCurrentPopup();
            }
            if (openPanel)
            {
                // bind panel: glued to the right of the context menu, aligned with the item
                int idx = isBind ? k : 0;
                gBindPanelPos = ImVec2(wp.x + size.x - U(5.5f), wp.y + U(3.5f) + ih * idx + U(4) - U(8));
                if (gBindPanelPos.x + U(311.5f) > ImGui::GetIO().DisplaySize.x - 4.f) gBindPanelPos.x = wp.x - U(311.5f) + U(5.5f);
                gBindPanelRequest = true;
            }
            y += ih;
            ImGui::PopID();
        }
        BindPanel();
        PopupEnd();
    }
}

static void GearPopup()
{
    if (gGearRequest) { ImGui::OpenPopup("##gear"); gGearRequest = false; }
    if (!gGearCfg) return;
    ImVec2 size(U(300), U(244));
    if (PopupBegin("##gear", ImVec2(gGearAnchor.x - size.x * 0.5f, gGearAnchor.y - size.y * 0.5f), size, 5.f))
    {
        ImVec2 wp = ImGui::GetWindowPos();
        CardBegin(gGearTitle.c_str(), wp, size.x, false, 14.f, 20.5f, 33.5f);
        RowToggle(ICON_FA_PALETTE, "Theme Colors", &gGearCfg->themeColors);
        RowSlider(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER, "Size", &gGearCfg->size, 0.f, 72.f, "%.0fpx", 56.f, 19.f, 40.f);
        RowSlider(ICON_FA_DROPLET, "Opacity", &gGearCfg->opacity, 0.f, 100.f, "%.0f%%", 56.f, 19.f, 40.f);
        RowSlider(ICON_FA_ARROW_POINTER, "Hover Opacity", &gGearCfg->hoverOpacity, 0.f, 100.f, "%.0f%%", 56.f, 19.f, 40.f);
        CardEnd();
        PopupEnd();
    }
}

static void BindListPopup()
{
    if (gBindListRequest) { ImGui::OpenPopup("##bindlist"); gBindListRequest = false; }
    int n = (int)gBinds.size();
    float rowh = U(34);
    ImVec2 size(U(420), U(46) + ImMax(1, n) * rowh + U(10));
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (PopupBegin("##bindlist", ImVec2(ds.x * 0.5f - size.x * 0.5f, ds.y * 0.5f - size.y * 0.5f), size, 6.f))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        Icon(dl, ICON_FA_LIST_UL, kIcon, ImVec2(wp.x + U(22), wp.y + U(22)), P.accent);
        TextL(dl, wp.x + U(40), wp.y + U(22), P.textBright, T("Bind List"));
        {
            ImVec2 c(wp.x + size.x - U(20), wp.y + U(22));
            bool p = Btn("##close", ImVec2(c.x - U(10), c.y - U(10)), ImVec2(c.x + U(10), c.y + U(10)));
            Icon(dl, ICON_FA_XMARK, 15.f, c, ImGui::IsItemHovered() ? P.textBright : P.text);
            if (p) ImGui::CloseCurrentPopup();
        }
        dl->AddLine(ImVec2(wp.x, wp.y + U(44)), ImVec2(wp.x + size.x, wp.y + U(44)), P.popupSep);
        if (n == 0) TextL(dl, wp.x + U(20), wp.y + U(44) + rowh * 0.5f + U(4), P.title, T("No binds yet"));
        for (int i = 0; i < n; i++)
        {
            Bind& b = gBinds[i];
            float cy = wp.y + U(48) + rowh * (i + 0.5f);
            ImU32 dot = (b.target && *b.target) ? P.accent : P.iconOff;
            dl->AddCircleFilled(ImVec2(wp.x + U(22), cy), U(3.5f), dot);
            TextL(dl, wp.x + U(36), cy, P.text, b.label.c_str(), 16.f, wp.x + size.x - U(150), U(12));
            std::string k = std::string(KeyName(b.key)) + "  " + T(kBindType[b.type]);
            TextR(dl, wp.x + size.x - U(20), cy, P.textBright, k.c_str(), 16.f);
        }
        PopupEnd();
    }
}

static void ColorPickPopup()
{
    if (gColorPickRequest) { ImGui::OpenPopup("##colorpick"); gColorPickRequest = false; }
    ImVec2 size(U(230), U(250));
    ImVec2 pos = gColorPickPos;
    if (PopupBegin("##colorpick", pos, size, 5.f))
    {
        ImVec2 wp = ImGui::GetWindowPos();
        TextL(ImGui::GetWindowDrawList(), wp.x + U(12), wp.y + U(16), P.title, T("Custom color"));
        ImGui::SetCursorScreenPos(ImVec2(wp.x + U(12), wp.y + U(30)));
        ImGui::PushFont(FText, U(13));
        ImGui::SetNextItemWidth(size.x - U(24));
        if (ImGui::ColorPicker3("##cp", &st.customAccent.x, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar))
        {
            st.theme = 6;
            BuildPalette(st.theme, ImGui::ColorConvertFloat4ToU32(st.customAccent));
        }
        ImGui::PopFont();
        PopupEnd();
    }
}

static void GlobalPopups()
{
    ContextMenu();
    GearPopup();
    BindListPopup();
    ColorPickPopup();
}

// ---------------------------------------------------------------------------
// Logo
// ---------------------------------------------------------------------------
static void Logo(ImDrawList* dl, ImVec2 c, float r)
{
    ImU32 red = P.accent, white = RGB(228, 228, 225);
    for (int i = 0; i < 8; i++)
    {
        float a0 = -IM_PI * 0.5f + i * IM_PI / 4.f, a1 = a0 + IM_PI / 4.f, am = (a0 + a1) * 0.5f;
        ImVec2 p0(c.x + cosf(a0) * r, c.y + sinf(a0) * r), p1(c.x + cosf(a1) * r, c.y + sinf(a1) * r);
        ImVec2 ctrl(c.x + cosf(am) * r * 0.80f, c.y + sinf(am) * r * 0.80f);
        ImVec2 pts[16]; int n = 0;
        pts[n++] = c;
        const int seg = 10;
        for (int k = 0; k <= seg; k++)
        {
            float t = k / (float)seg, u = 1 - t;
            pts[n++] = ImVec2(u * u * p0.x + 2 * u * t * ctrl.x + t * t * p1.x, u * u * p0.y + 2 * u * t * ctrl.y + t * t * p1.y);
        }
        dl->AddConvexPolyFilled(pts, n, (i % 2 == 0) ? red : white);
    }
}

// ---------------------------------------------------------------------------
// Main menu window
// ---------------------------------------------------------------------------
static void HeroesOverlayPage(float cx, float cy, float cw)
{
    float gapX = U(13), gapY = U(14.5f);
    float colW = (cw - gapX) * 0.5f;
    float lx = cx, rx = cx + colW + gapX;

    float h1, h2;
    CardBegin("Items Overlay", ImVec2(lx, cy), colW);
    RowToggle(ICON_FA_CHECK, "Enable", &st.itemsEnable, Gear("items.enable"));
    {
        ImGui::PushID("Settings");
        Row r = RowBegin(38.f);
        Btn("##row", r.a, r.b, true);
        DrawIcon(C.dl, ICON_FA_SLIDERS, ImVec2(r.iconX, r.cy), P.accent);
        TextL(C.dl, r.labelX, r.cy, P.text, T("Settings"));
        GearButton("##gear", ImVec2(r.right - U(12.5f), r.cy), Gear("items.settings"), "Items Overlay");
        ImGui::PopID();
    }
    RowCombo(ICON_FA_PERSON_CIRCLE_QUESTION, "Show On", &st.itemsShowOn, kShowOn, 3, true);
    RowCombo(ICON_FA_ALIGN_LEFT, "Align", &st.itemsAlign, kAlign, 4, false, Gear("items.align"));
    h1 = CardEnd();

    CardBegin("Skills Overlay", ImVec2(rx, cy), colW);
    RowToggle(ICON_FA_CHECK, "Enable", &st.skillsEnable, Gear("skills.enable"));
    RowToggle("@ooo", "Minified", &st.skillsMinified);
    RowCombo(ICON_FA_PERSON_CIRCLE_QUESTION, "Show On", &st.skillsShowOn, kShowOn, 3, true);
    RowCombo(ICON_FA_ALIGN_LEFT, "Align", &st.skillsAlign, kAlign, 4, false, Gear("skills.align"));
    h2 = CardEnd();

    float y2 = cy + ImMax(h1, h2) + gapY;
    CardBegin("Modifiers Overlay", ImVec2(lx, y2), colW);
    RowToggle(ICON_FA_CHECK, "Enable", &st.modEnable, Gear("mod.enable"));
    RowCombo(ICON_FA_PERSON_CIRCLE_QUESTION, "Show On", &st.modShowOn, kShowOn, 3, true);
    RowCombo(ICON_FA_ALIGN_LEFT, "Align", &st.modAlign, kAlign, 4, false, Gear("mod.align"));
    h1 = CardEnd();

    CardBegin("Bars Overlay", ImVec2(rx, y2), colW);
    RowToggle(ICON_FA_CHECK, "Enable", &st.barsEnable);
    RowCombo(ICON_FA_TOGGLE_ON, "Custom Bars", &st.barsCustom, kBars, 2, false);
    RowToggle("@exp", "Experience", &st.barsExp, Gear("bars.exp"));
    h2 = CardEnd();

    float y3 = y2 + ImMax(h1, h2) + gapY;
    CardBegin("Extra Settings", ImVec2(lx, y3), cw);
    RowSlider(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER, "OnHover Radius", &st.hoverRadius, 80.f, 1000.f, "%.0fpx");
    CardEnd();
}

static void EmptyPage(float cx, float cy, float cw, const char* name)
{
    CardBegin(name, ImVec2(cx, cy), cw);
    Row r = RowBegin(48.f, true);
    DrawIcon(C.dl, ICON_FA_CIRCLE_CHEVRON_DOWN, ImVec2(r.iconX, r.cy), P.iconOff);
    TextL(C.dl, r.labelX, r.cy, P.text, T("This module has no options yet."));
    CardEnd();
}

static void MenuWindow()
{
    ImVec2 size(U(838), U(559));
    ImGui::SetNextWindowPos(gMenuPos, gPosSet ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##umbrella_menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    O = ImGui::GetWindowPos();
    gMenuPos = O;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float R = U(6);
    ImVec2 A = O, B(O.x + size.x, O.y + size.y);

    // frame
    Shadow(dl, A, B, R, 1.4f);
    dl->AddRectFilled(A, B, P.content, R);
    dl->AddRectFilled(A, Pt(49, 559), P.rail, R, ImDrawFlags_RoundCornersLeft);
    dl->AddRectFilled(Pt(49, 0), Pt(211.5f, 559), P.side);
    dl->AddLine(Pt(49, 0), Pt(49, 559), P.border1, 1.f);
    dl->AddLine(Pt(211.5f, 0), Pt(211.5f, 559), P.border2, 1.f);
    dl->AddRect(A, B, P.outer, R, 0, 1.f);

    // ---------------- rail
    Logo(dl, Pt(24.5f, 25.f), U(15.f));
    for (int i = 0; i < 9; i++)
    {
        ImGui::PushID(i);
        ImVec2 c = Pt(24.5f, 75.5f + 42.f * i);
        ImVec2 a(c.x - U(17.5f), c.y - U(16.25f)), b(c.x + U(17.5f), c.y + U(16.25f));
        bool pressed = Btn("##rail", a, b);
        bool hov = ImGui::IsItemHovered();
        if (hov) ImGui::SetTooltip("%s", T(kRail[i].name));
        if (pressed) { gRail = i; gPage = (i == 3) ? 2 : 0; }
        float s = Anim(ImGui::GetItemID(), gRail == i ? 1.f : 0.f);
        float hv = Anim(ImGui::GetItemID() + 1, hov ? 1.f : 0.f);
        if (s > 0.01f || hv > 0.01f) dl->AddRectFilled(a, b, WithA(P.sel, ImMax(s, hv * 0.6f)), U(4));
        if (s > 0.01f) dl->AddRectFilled(ImVec2(O.x + U(2.5f), c.y - U(6) * s), ImVec2(O.x + U(4.5f), c.y + U(6) * s), P.accent, U(1));
        DrawIcon(dl, kRail[i].icon, c, LerpC(P.railIcon, P.accent, s), 16.f);
        ImGui::PopID();
    }
    {
        ImVec2 c = Pt(24.5f, 559 - 32.5f);
        bool pressed = Btn("##railgear", ImVec2(c.x - U(17.5f), c.y - U(16)), ImVec2(c.x + U(17.5f), c.y + U(16)));
        float hv = Anim(ImGui::GetItemID(), (ImGui::IsItemHovered() || gSettingsOpen) ? 1.f : 0.f);
        if (pressed) gSettingsOpen = !gSettingsOpen;
        DrawIcon(dl, ICON_FA_GEAR, c, LerpC(P.railIcon, P.textBright, hv), 16.f);
    }

    // ---------------- sidebar
    const char* catName = kRail[gRail].name;
    TextL(dl, Pt(66, 0).x, Pt(0, 34).y, P.sideText, T(catName));
    if (gRail == 3)
    {
        std::string q = Lower(gSearch);
        int vis = 0;
        for (int i = 0; i < 10; i++)
        {
            if (!q.empty() && Lower(T(kSide[i].name)).find(q) == std::string::npos && Lower(kSide[i].name).find(q) == std::string::npos) continue;
            ImGui::PushID(i);
            float cy = 76.5f + 33.f * vis;
            ImVec2 a = Pt(60.5f, cy - 13.75f), b = Pt(200.5f, cy + 13.75f);
            bool pressed = Btn("##side", a, b);
            bool hov = ImGui::IsItemHovered();
            if (pressed) gPage = i;
            float s = Anim(ImGui::GetItemID(), gPage == i ? 1.f : 0.f);
            float hv = Anim(ImGui::GetItemID() + 1, hov ? 1.f : 0.f);
            if (s > 0.01f || hv > 0.01f) dl->AddRectFilled(a, b, WithA(P.sel, ImMax(s, hv * 0.5f)), U(3.5f));
            if (s > 0.01f) dl->AddRectFilled(Pt(53.5f, cy - 6 * s), Pt(55.5f, cy + 6 * s), P.accent, U(1));
            ImU32 col = LerpC(LerpC(P.sideText, P.textBright, hv * 0.35f), P.accent, s);
            DrawIcon(dl, kSide[i].icon, Pt(76, cy), col, 16.f);
            TextL(dl, Pt(94, 0).x, Pt(0, cy).y, col, T(kSide[i].name), kText, Pt(195, 0).x, U(13));
            ImGui::PopID();
            vis++;
        }
    }

    // ---------------- header
    const char* pageName = gRail == 3 ? kSide[gPage].name : kRail[gRail].name;
    {
        float x = Pt(222, 0).x, cy = Pt(0, 34).y;
        TextL(dl, x, cy, P.crumb, T("Main")); x += TextW(T("Main")) + U(9);
        TextL(dl, x, cy, P.crumb, "/"); x += TextW("/") + U(10);
        TextL(dl, x, cy, P.accent, T(pageName));
        Icon(dl, ICON_FA_BOOK, kIcon, Pt(572, 34), P.hdrIcon);
        Icon(dl, ICON_FA_CLOUD, kIcon, Pt(595, 34.5f), P.hdrIcon);
        ImVec2 sa = Pt(614, 20.5f), sb = Pt(827, 48);
        dl->AddRectFilled(sa, sb, P.card, U(4));
        dl->AddRect(sa, sb, P.searchBorder, U(4), 0, 1.f);
        Icon(dl, ICON_FA_MAGNIFYING_GLASS, kIcon, Pt(628.5f, 34), P.placeholder);
        ImGui::SetCursorScreenPos(ImVec2(Pt(641.5f, 0).x, sa.y));
        ImGui::PushFont(FText, U(kText));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, (sb.y - sa.y - U(kText)) * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, P.text);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, P.placeholder);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, WithA(P.accent, 0.35f));
        ImGui::SetNextItemWidth(sb.x - U(8) - Pt(641.5f, 0).x);
        ImGui::InputTextWithHint("##search", T("Search"), gSearch, sizeof(gSearch));
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }

    // ---------------- content
    float cx = Pt(222, 0).x, cy = Pt(0, 64).y, cw = U(827 - 222);
    if (gRail == 3 && gPage == 2) HeroesOverlayPage(cx, cy, cw);
    else EmptyPage(cx, cy, cw, pageName);

    GlobalPopups();
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Settings window
// ---------------------------------------------------------------------------
static void SettingsWindow()
{
    ImVec2 size(U(323), U(727));
    ImGui::SetNextWindowPos(gSettingsPos, gPosSet ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##umbrella_settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    O = ImGui::GetWindowPos();
    gSettingsPos = O;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 A = O, B(O.x + size.x, O.y + size.y);
    Shadow(dl, A, B, U(6), 1.4f);
    dl->AddRectFilled(A, B, P.win, U(6));
    dl->AddRect(A, B, P.winBorder, U(6), 0, 1.f);
    // title bar
    DrawIcon(dl, "@folder", Pt(14.5f, 14.5f), P.text, 15.f);
    TextL(dl, Pt(26, 0).x, Pt(0, 15).y, LerpC(P.text, P.textBright, 0.3f), T("Settings"));
    {
        ImVec2 c = Pt(312, 14.5f);
        bool p = Btn("##close", ImVec2(c.x - U(10), c.y - U(10)), ImVec2(c.x + U(10), c.y + U(10)));
        Icon(dl, ICON_FA_XMARK, 15.f, c, ImGui::IsItemHovered() ? P.textBright : P.text);
        if (p) gSettingsOpen = false;
    }
    dl->AddLine(Pt(0, 27), Pt(323, 27), P.winBorder, 1.f);

    float cw = U(294);
    CardBegin("Main", Pt(14.5f, 41.5f), cw);
    RowKey(ICON_FA_BOX_OPEN, "Menu Bind", &st.menuKey);
    RowKey(ICON_FA_ROTATE_RIGHT, "Reload Bind", &st.reloadKey);
    RowToggle(ICON_FA_FILE_PEN, "Log Window", &st.logWindow, Gear("settings.log"));
    RowToggle(ICON_FA_UMBRELLA_BEACH, "Binds Island", &st.bindsIsland);
    RowCombo("@0110", "Enable Scripts", &st.scripts, kScripts, 3, true);
    if (RowCombo(ICON_FA_LANGUAGE, "Language", &st.lang, kLang, 2, false)) gLang = st.lang;
    RowSlider(ICON_FA_CLOCK, "Animation Duration", &st.animDur, 10.f, 250.f, "%.0f%%", 55.f, 18.f, 39.f);
    RowToggle("@curly", "Developer Mode", &st.devMode);
    float h = CardEnd();

    CardBegin("Visual", Pt(14.5f, 41.5f) + ImVec2(0, h + U(14.5f)), cw);
    RowSlider("@drop", "Menu Blur Factor", &st.blur, 0.30f, 1.f, "%.2f", 54.5f, 18.f, 38.5f);
    RowToggle(ICON_FA_CIRCLE_HALF_STROKE, "Disable Shadows", &st.disableShadows);
    RowCombo(ICON_FA_EXPAND, "Menu Scale", &st.scaleIdx, kScaleNames, 5, false);
    RowToggle(ICON_FA_PAINTBRUSH, "Enable Visuals", &st.enableVisuals);
    {
        ImGui::PushID("notif");
        Row r = RowBegin(38.f);
        bool p = Btn("##row", r.a, r.b);
        bool hov = ImGui::IsItemHovered();
        DrawIcon(C.dl, ICON_FA_BELL, ImVec2(r.iconX, r.cy), P.accent);
        TextL(C.dl, r.labelX, r.cy, P.text, T("Notifications"));
        Icon(C.dl, ICON_FA_BARS, 15.f, ImVec2(r.right - U(7), r.cy), hov ? P.textBright : LerpC(P.text, P.textBright, 0.25f));
        if (p) Notify(T("Notifications are enabled"));
        ImGui::PopID();
    }
    {
        ImGui::PushID("theme");
        Row r = RowBegin(45.f, false);
        TextL(C.dl, r.labelX, r.cy - U(1.5f), P.text, T("Theme"));
        for (int i = 0; i < 7; i++)
        {
            ImGui::PushID(i);
            ImVec2 c(O.x + U(98.5f + 22.f * i), r.cy);
            bool p = Btn("##sw", ImVec2(c.x - U(10), c.y - U(10)), ImVec2(c.x + U(10), c.y + U(10)));
            bool hov = ImGui::IsItemHovered();
            bool sel = st.theme == i;
            float sa = Anim(ImGui::GetItemID(), sel ? 1.f : 0.f);
            float ha = Anim(ImGui::GetItemID() + 7, hov ? 1.f : 0.f);
            if (i < 6)
            {
                ThemeDef& td = gThemes[i];
                C.dl->AddCircleFilled(c, U(7.5f + ha * 0.5f), td.swatchBg, 32);
                if (sa > 0.01f) C.dl->AddCircle(c, U(9.f), WithA(td.accent, sa), 40, U(1.3f));
                C.dl->AddCircleFilled(c, U(4.3f), td.accent, 24);
                if (p) { st.theme = i; BuildPalette(st.theme, 0); }
            }
            else
            {
                C.dl->AddCircleFilled(c, U(8.f), LerpC(RGB(36, 36, 36), RGB(50, 50, 50), ha), 32);
                if (sa > 0.01f) C.dl->AddCircle(c, U(9.5f), WithA(ImGui::ColorConvertFloat4ToU32(st.customAccent), sa), 40, U(1.3f));
                Icon(C.dl, ICON_FA_PLUS, 11.f, c, RGB(175, 175, 175));
                if (p)
                {
                    gColorPickPos = ImVec2(c.x - U(115), c.y + U(14));
                    gColorPickRequest = true;
                }
            }
            if (hov) ImGui::SetTooltip("%s", i == 6 ? T("Custom color") : T("Theme"));
            ImGui::PopID();
        }
        ImGui::PopID();
    }
    CardEnd();

    GlobalPopups();
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Extras: log window, binds island, toasts, backdrop
// ---------------------------------------------------------------------------
static void LogWindow()
{
    ImVec2 size(U(360), U(220));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - size.x - U(20), ImGui::GetIO().DisplaySize.y - size.y - U(20)), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##umbrella_log", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    O = ImGui::GetWindowPos();
    GearCfg* g = Gear("settings.log");
    float alpha = g->opacity / 100.f;
    if (ImGui::IsWindowHovered()) alpha = ImMax(alpha, g->hoverOpacity / 100.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    int v0 = dl->VtxBuffer.Size;
    ImVec2 A = O, B(O.x + size.x, O.y + size.y);
    dl->AddRectFilled(A, B, P.win, U(6));
    dl->AddRect(A, B, P.winBorder, U(6), 0, 1.f);
    DrawIcon(dl, ICON_FA_FILE_PEN, Pt(15, 14.5f), g->themeColors ? P.accent : P.text);
    TextL(dl, Pt(30, 0).x, Pt(0, 15).y, P.text, T("Log"));
    if (st.devMode)
    {
        char buf[48]; snprintf(buf, sizeof(buf), "%.0f FPS", ImGui::GetIO().Framerate);
        TextR(dl, Pt(345, 0).x, Pt(0, 15).y, P.title, buf, 15.f);
    }
    dl->AddLine(Pt(0, 27), Pt(360, 27), P.winBorder, 1.f);
    float fs = ImMax(10.f, g->size * 0.5f);
    float lh = U(fs + 4);
    int maxLines = (int)((size.y - U(36)) / lh);
    int start = ImMax(0, (int)gLog.size() - maxLines);
    dl->PushClipRect(Pt(0, 28), B, true);
    for (int i = start; i < (int)gLog.size(); i++)
        TextL(dl, Pt(12, 0).x, Pt(0, 36).y + lh * (i - start) + lh * 0.5f - U(4), P.text, gLog[i].c_str(), fs);
    dl->PopClipRect();
    for (int i = v0; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].col = WithA(dl->VtxBuffer[i].col, alpha);
    ImGui::Dummy(size);
    ImGui::End();
}

static void Island()
{
    if (!st.bindsIsland) return;
    std::vector<int> v;
    for (int i = 0; i < (int)gBinds.size(); i++) if (gBinds[i].visible && gBinds[i].key != ImGuiKey_None) v.push_back(i);
    float t = Anim(0x15A1A9D, v.empty() ? 0.f : 1.f, 10.f);
    if (t < 0.01f) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float rowh = U(22);
    float w = U(60);
    for (int i : v) w = ImMax(w, TextW(gBinds[i].label.c_str(), 14.f) + TextW(KeyName(gBinds[i].key), 14.f) + U(70));
    float h = U(14) + rowh * ImMax(1, (int)v.size());
    ImVec2 a(ds.x * 0.5f - w * 0.5f, U(10) - (1 - t) * U(20)), b(a.x + w, a.y + h);
    O = ImVec2(0, 0);
    int v0 = dl->VtxBuffer.Size;
    if (!st.disableShadows) for (int k = 0; k < 8; k++) dl->AddRectFilled(a - ImVec2(k, k - 3), b + ImVec2(k, k + 3), IM_COL32(0, 0, 0, 10), h * 0.5f + k);
    dl->AddRectFilled(a, b, IM_COL32(8, 8, 8, 240), ImMin(h * 0.5f, U(16)));
    dl->AddRect(a, b, P.popupBorder, ImMin(h * 0.5f, U(16)));
    for (int k = 0; k < (int)v.size(); k++)
    {
        Bind& bd = gBinds[v[k]];
        float cy = a.y + U(7) + rowh * (k + 0.5f);
        bool on = bd.target ? *bd.target : bd.value;
        dl->AddCircleFilled(ImVec2(a.x + U(16), cy), U(3.5f), on ? P.accent : P.iconOff);
        TextL(dl, a.x + U(28), cy, RGB(200, 200, 200), bd.label.c_str(), 14.f);
        const char* kn = KeyName(bd.key);
        float kw = TextW(kn, 13.f) + U(10);
        ImVec2 ka(b.x - U(12) - kw, cy - U(8)), kb(b.x - U(12), cy + U(8));
        dl->AddRectFilled(ka, kb, RGB(36, 39, 44), U(3));
        TextL(dl, ka.x + U(5), cy, RGB(190, 190, 190), kn, 13.f);
    }
    for (int i = v0; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].col = WithA(dl->VtxBuffer[i].col, t);
}

static void Toasts()
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float y = ds.y - U(20);
    for (int i = (int)gToasts.size() - 1; i >= 0; i--)
    {
        Toast& t = gToasts[i];
        t.t += ImGui::GetIO().DeltaTime;
        float life = 3.2f;
        float a = ImSaturate(t.t / 0.2f) * ImSaturate((life - t.t) / 0.4f);
        float w = TextW(t.text.c_str(), 15.f) + U(52), h = U(36);
        ImVec2 pa(ds.x - U(20) - w + (1 - a) * U(30), y - h), pb(pa.x + w, y);
        int v0 = dl->VtxBuffer.Size;
        dl->AddRectFilled(pa, pb, P.popup, U(6));
        dl->AddRect(pa, pb, P.popupBorder, U(6));
        dl->AddRectFilled(ImVec2(pa.x, pa.y + U(8)), ImVec2(pa.x + U(2.5f), pb.y - U(8)), P.accent, U(1));
        Icon(dl, ICON_FA_BELL, 14.f, ImVec2(pa.x + U(20), (pa.y + pb.y) * 0.5f), P.accent);
        TextL(dl, pa.x + U(36), (pa.y + pb.y) * 0.5f, P.textBright, t.text.c_str(), 15.f);
        for (int k = v0; k < dl->VtxBuffer.Size; k++) dl->VtxBuffer[k].col = WithA(dl->VtxBuffer[k].col, a);
        y -= h + U(8);
        if (t.t > life) gToasts.erase(gToasts.begin() + i);
    }
}

static void Backdrop()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ds, P.backdropTop, P.backdropTop, P.backdropBottom, P.backdropBottom);
    // soft warm glows (imitating a blurred game scene behind the menu)
    if (st.enableVisuals)
    {
        float tt = (float)ImGui::GetTime() * 0.15f;
        struct G { float x, y, r; ImU32 c; } gl[] = {
            { 0.25f, 0.30f, 0.45f, IM_COL32(90, 50, 30, 18) }, { 0.78f, 0.22f, 0.40f, IM_COL32(60, 40, 70, 16) },
            { 0.60f, 0.85f, 0.50f, IM_COL32(30, 50, 60, 16) },
        };
        for (auto& g : gl)
        {
            ImVec2 c(ds.x * (g.x + 0.03f * sinf(tt + g.y * 7)), ds.y * (g.y + 0.03f * cosf(tt + g.x * 5)));
            float r = ImMax(ds.x, ds.y) * g.r;
            for (int k = 0; k < 10; k++) dl->AddCircleFilled(c, r * (1.f - k * 0.09f), g.c, 64);
        }
    }
    // dim by blur factor
    dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, (int)(st.blur * 120)));
    if (!gVisible)
    {
        O = ImVec2(0, 0);
        std::string s = std::string(T("Press")) + " " + KeyName(st.menuKey) + " " + T("to open the menu");
        float w = TextW(s.c_str());
        TextL(dl, ds.x * 0.5f - w * 0.5f, ds.y * 0.5f, RGB(150, 150, 150), s.c_str());
    }
}

// ---------------------------------------------------------------------------
// Input: key binds
// ---------------------------------------------------------------------------
static void ProcessKeys()
{
    ImGuiIO& io = ImGui::GetIO();
    if (gListenKey)
    {
        // verify pointer still valid (binds may be erased)
        bool valid = gListenKey == &st.menuKey || gListenKey == &st.reloadKey;
        for (auto& b : gBinds) if (&b.key == gListenKey) valid = true;
        if (!valid) { gListenKey = nullptr; return; }
        for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++)
        {
            if (k == ImGuiKey_MouseLeft || k == ImGuiKey_MouseRight || k == ImGuiKey_MouseWheelX || k == ImGuiKey_MouseWheelY) continue;
            if (k >= ImGuiKey_ReservedForModCtrl && k <= ImGuiKey_ReservedForModSuper) continue;
            if (ImGui::IsKeyPressed((ImGuiKey)k, false))
            {
                *gListenKey = (k == ImGuiKey_Escape) ? (int)ImGuiKey_None : k;
                gListenKey = nullptr;
                return;
            }
        }
        return;
    }
    if (io.WantTextInput) return;
    if (st.menuKey != ImGuiKey_None && ImGui::IsKeyPressed((ImGuiKey)st.menuKey, false)) gVisible = !gVisible;
    if (st.reloadKey != ImGuiKey_None && ImGui::IsKeyPressed((ImGuiKey)st.reloadKey, false)) Notify(T("Scripts reloaded"));
    for (auto& b : gBinds)
    {
        if (b.key == ImGuiKey_None || !b.target) continue;
        switch (b.type)
        {
        case 0: if (ImGui::IsKeyPressed((ImGuiKey)b.key, false)) *b.target = !*b.target; break;
        case 1: *b.target = ImGui::IsKeyDown((ImGuiKey)b.key) ? b.value : !b.value; break;
        case 2: *b.target = b.value; break;
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void Menu::Init(int default_scale)
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 2; cfg.OversampleV = 1;
    FText = io.Fonts->AddFontFromMemoryTTF((void*)font_lato, (int)font_lato_size, 18.f, &cfg);
    FBold = io.Fonts->AddFontFromMemoryTTF((void*)font_lato_bold, (int)font_lato_bold_size, 18.f, &cfg);
    ImFontConfig icfg; icfg.FontDataOwnedByAtlas = false;
    FIcon = io.Fonts->AddFontFromMemoryTTF((void*)font_fa, (int)font_fa_size, 16.f, &icfg);
    io.FontDefault = FText;

    for (int i = 0; i < 5; i++) if (kScales[i] == default_scale) st.scaleIdx = i;
    stDefault = st;
    BuildPalette(st.theme, 0);
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark(&s);
    s.WindowRounding = 6; s.FrameRounding = 4; s.PopupRounding = 5; s.GrabRounding = 4;
    s.Colors[ImGuiCol_PopupBg] = ImVec4(0.086f, 0.086f, 0.086f, 1.f);
    s.Colors[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
    Log("Umbrella menu initialised");
}

void Menu::Frame()
{
    S = kScales[st.scaleIdx] / 100.f;
    ImGui::GetStyle().FontSizeBase = 15.f * S;
    gLang = st.lang;
    ProcessKeys();
    Backdrop();
    if (gVisible)
    {
        MenuWindow();
        if (gSettingsOpen) SettingsWindow();
    }
    if (st.logWindow) LogWindow();
    Island();
    Toasts();
    gPosSet = false;
}

void Menu::ClearColor(float* rgb)
{
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(P.backdropBottom);
    rgb[0] = c.x; rgb[1] = c.y; rgb[2] = c.z;
}

void Menu::RequiredSize(int scale, float* w, float* h)
{
    float s = scale / 100.f;
    *w = (838 + 16 + 323 + 80) * s;
    *h = (727 + 80) * s;
}

void Menu::SetWindowPositions(float mx, float my, float sx, float sy)
{
    gMenuPos = ImVec2(mx, my); gSettingsPos = ImVec2(sx, sy); gPosSet = true;
}
void Menu::SetSettingsOpen(bool open) { gSettingsOpen = open; }
void Menu::DebugOpen(int what)
{
    if (what == 1) { st.theme = 2; BuildPalette(2, 0); }
}
