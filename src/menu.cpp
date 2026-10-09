#include "hook.h"
#include "theme.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "hud.h"
#include "top_anchor.h"
#include "ui_icons.h"
#include "skin_changer.h"
#include "auto_accept.h"
#include "kill_stealer.h"
#include "armlet.h"
#include "combos.h"
#include "draft_advisor_ui.h"
#include <cctype>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cwchar>
#include <initializer_list>
#include <algorithm>

static ImVec4 UI_RGB(int r,int g,int b){return ImVec4(r/255.f,g/255.f,b/255.f,1.f);}
void theme::ApplyStyle() {
    auto& s=ImGui::GetStyle();s.Alpha=1.f;
    s.WindowRounding=5;s.ChildRounding=4;s.FrameRounding=3;s.PopupRounding=4;
    s.ScrollbarRounding=3;s.GrabRounding=3;s.TabRounding=3;
    s.WindowBorderSize=1;s.ChildBorderSize=1;s.FrameBorderSize=0;
    s.WindowPadding=ImVec2(16,14);s.FramePadding=ImVec2(10,7);
    s.ItemSpacing=ImVec2(12,8);s.ItemInnerSpacing=ImVec2(8,6);s.ScrollbarSize=5;
    auto& c=s.Colors;
    c[ImGuiCol_Text]=UI_RGB(172,176,185);
    c[ImGuiCol_TextDisabled]=UI_RGB(131,135,144);
    c[ImGuiCol_WindowBg]=UI_RGB(16,17,21);
    c[ImGuiCol_ChildBg]=UI_RGB(20,21,25);
    c[ImGuiCol_PopupBg]=UI_RGB(20,21,25);
    c[ImGuiCol_Border]=UI_RGB(24,26,31);
    c[ImGuiCol_FrameBg]=UI_RGB(24,27,32);
    c[ImGuiCol_FrameBgHovered]=UI_RGB(30,34,40);
    c[ImGuiCol_FrameBgActive]=UI_RGB(36,41,49);
    c[ImGuiCol_TitleBg]=c[ImGuiCol_TitleBgActive]=c[ImGuiCol_WindowBg];
    c[ImGuiCol_Header]=UI_RGB(25,30,35);
    c[ImGuiCol_HeaderHovered]=UI_RGB(30,36,43);
    c[ImGuiCol_HeaderActive]=UI_RGB(34,42,50);
    c[ImGuiCol_Button]=UI_RGB(26,30,36);
    c[ImGuiCol_ButtonHovered]=UI_RGB(34,39,47);
    c[ImGuiCol_ButtonActive]=UI_RGB(41,48,58);
    c[ImGuiCol_CheckMark]=UI_RGB(32,157,212);
    c[ImGuiCol_SliderGrab]=c[ImGuiCol_NavCursor]=c[ImGuiCol_CheckMark];
    c[ImGuiCol_SliderGrabActive]=UI_RGB(77,180,225);
    c[ImGuiCol_Separator]=UI_RGB(32,34,40);
    c[ImGuiCol_ScrollbarBg]=c[ImGuiCol_ChildBg];
    c[ImGuiCol_ScrollbarGrab]=UI_RGB(51,55,64);
    c[ImGuiCol_ScrollbarGrabHovered]=UI_RGB(68,73,85);
    c[ImGuiCol_Tab]=c[ImGuiCol_FrameBg];c[ImGuiCol_TabSelected]=c[ImGuiCol_Header];
    c[ImGuiCol_TabHovered]=c[ImGuiCol_HeaderHovered];
    c[ImGuiCol_TextSelectedBg]=UI_RGB(37,62,77);
}
static bool s_capture=false;
static int s_openReq=-1;
static void RebindPopup(int bi) {
    auto& b=binds::items[bi];ImGui::Text("%s / %s",b.name,binds::KeyName(b.vk));
    if(s_capture){
        ImGui::TextUnformatted("Press a key / ESC to cancel");
        for(int k=3;k<256;++k)if(k!=VK_ESCAPE&&!(bi==2&&k==VK_F8)&&binds::Edge[k]){b.vk=k;s_capture=false;break;}
        if(binds::Edge[VK_ESCAPE])s_capture=false;
    } else {
        if(ImGui::Button("Change key"))s_capture=true;
        ImGui::SameLine();if(ImGui::Button("Clear"))b.vk=0;
    }
    if(bi!=0&&bi!=10)ImGui::Checkbox("Hold",&b.hold);else{b.hold=true;ImGui::TextDisabled("%s: hold mode only",b.name);}
    if(ImGui::Button("Done"))ImGui::CloseCurrentPopup();
}
namespace clientui {
static int language = 0;
bool English(){return language!=0;}
static int accent = 0;
static float fontScale = 1.f;
static int profile = 0;
static const wchar_t* profileNames[] = { L"default", L"practice", L"custom" };
static const char* profileLabels[] = { "default", "practice", "custom" };
static char message[160] = {};
static bool initialized = false;

static const char* T(const char* ru, const char* en) { return language == 0 ? ru : en; }

static const wchar_t* ConfigPath() {
    static wchar_t path[MAX_PATH] = {};
    if (path[0]) return path;
    HMODULE mod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ConfigPath), &mod)) return L"";
    DWORD n = GetModuleFileNameW(mod, path, MAX_PATH);
    if (!n || n >= MAX_PATH) { path[0] = 0; return path; }
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) { path[0] = 0; return path; }
    *(slash + 1) = 0;
    const wchar_t* name = L"fda-client.ini";
    if (wcslen(path) + wcslen(name) >= MAX_PATH) { path[0] = 0; return path; }
    wcscat_s(path, name);
    return path;
}

struct BoolSetting { const wchar_t* key; bool* target; };
static BoolSetting booleans[] = {
    {L"armletAuto", &cfg::armletAuto}, {L"showEffects", &cfg::showEffects}, {L"effectsTimedOnly", &cfg::effectsTimedOnly},
    {L"showKillHelper", &cfg::showKillHelper}, {L"helperModeledOnly", &cfg::helperModeledOnly},
    {L"killStealer", &cfg::killStealer}, {L"autoAccept", &cfg::autoAccept},
    {L"ksQuickcastConfirmed", &cfg::ksQuickcastConfirmed},
    {L"lastHitConservative", &cfg::lastHitConservative},
    {L"notifyRunes", &cfg::notifyRunes}, {L"notifyRuneSoon", &cfg::notifyRuneSoon},
    {L"notifyWards", &cfg::notifyWards}, {L"notifyRoshan", &cfg::notifyRoshan},
    {L"visibleByEnemy", &cfg::hudVisibleByEnemy}, {L"farmHarass", &cfg::farmHarass}, {L"lastHitEnabled", &cfg::lastHitEnabled},
    {L"hudTopAbilities", &cfg::hudTopAbilities}, {L"hudStatusBadges", &cfg::hudStatusBadges},
    {L"hudTop", &cfg::hudTop}, {L"hudItems", &cfg::hudItems},
    {L"hudAbilities", &cfg::hudAbilities}, {L"hudRoshan", &cfg::hudRoshan}, {L"hudWatermark", &cfg::hudWatermark},
    {L"hudBounty", &cfg::hudBounty}, {L"hudPortraits", &cfg::hudPortraits}, {L"hudHpNumber", &cfg::hudHpNumber},
    {L"hudManaNumber", &cfg::hudManaNumber}, {L"hudIllusions", &cfg::hudIllusions},
    {L"espHeroes", &cfg::espHeroes},
    {L"espCds", &cfg::espCds}, {L"espDist", &cfg::espDist},
    {L"skillPreview", &cfg::skillPreview}, {L"espWards", &cfg::espWards},
    {L"espRoshan", &cfg::espRoshan}, {L"lastHit", &cfg::lastHit},
    {L"deny", &cfg::deny}, {L"vision", &cfg::vbe}, {L"glow", &cfg::glow},
    {L"infoPanel", &cfg::dotaPlus}, {L"dodgeAlert", &cfg::dodger},
    {L"autoDodge", &cfg::autoDodge}, 
    {L"farmAuto", &cfg::farmAuto}, {L"keybinds", &cfg::showKeybinds}
};
static bool WriteNumber(const wchar_t* key, int value) {
    wchar_t number[32]; swprintf_s(number, L"%d", value);
    return WritePrivateProfileStringW(profileNames[profile], key, number, ConfigPath()) != FALSE;
}
struct FloatSetting { const wchar_t* key; float* target; float lo,hi; };
static FloatSetting floats[] = {
    {L"armletThreshold", &cfg::armletThreshold,50.f,550.f}, {L"helperMargin", &cfg::helperMargin,0.f,1000.f}, {L"ksDamage", &cfg::ksDamage,0.f,100000.f}, {L"ksRange", &cfg::ksRange,0.f,2500.f}, {L"ksMargin", &cfg::ksMargin,0.f,1000.f},
    {L"notifyLead", &cfg::notifyLead,3.f,30.f},
    {L"hudSkillPixels", &cfg::hudSkillPixels,28.f,60.f},
    {L"nativeHpOffsetX", &cfg::nativeHpOffsetX,-100.f,100.f}, {L"nativeHpOffsetY", &cfg::nativeHpOffsetY,-100.f,100.f},
    {L"nativeHpRedOffsetY", &cfg::nativeHpRedOffsetY,-100.f,100.f},
    {L"hudIconScale", &cfg::hudIconScale, .6f,1.6f}, {L"hudTopY", &cfg::hudTopY,0.f,260.f},
    {L"hudTopSlot", &cfg::hudTopSlot,40.f,100.f}, {L"hudTopGap", &cfg::hudTopGap,100.f,320.f},
    {L"wmX", &cfg::hudWatermarkX,-1.f,16000.f},{L"wmY", &cfg::hudWatermarkY,0.f,16000.f},
    {L"roshanX", &cfg::hudRoshanX,-1.f,16000.f},{L"roshanY", &cfg::hudRoshanY,0.f,16000.f}
};
static bool Save() {
    if (!ConfigPath()[0]) return false;
    bool ok = WriteNumber(L"uiVersion",12);
    for(const auto& setting: {std::pair<const wchar_t*,int>{L"armletKey",cfg::armletKey},{L"armletSlot",cfg::armletSlot},{L"ksSlot",cfg::ksAbilitySlot},{L"ksSpellKey",cfg::ksSpellKey},{L"ksType",cfg::ksDamageType}})if(!WriteNumber(setting.first,setting.second))ok=false;
    for (const auto& b : booleans) if (!WriteNumber(b.key, *b.target ? 1 : 0)) ok = false;
    if (!WriteNumber(L"language", language)) ok = false;
    if (!WriteNumber(L"accent", accent)) ok = false;
    if (!WriteNumber(L"scale", (int)(fontScale * 100.f + 0.5f))) ok = false;
    for (int i = 0; i < binds::kCount; ++i) {
        wchar_t key[32]; swprintf_s(key, L"bind%d", i);
        if (!WriteNumber(key, binds::items[i].vk)) ok = false;
        swprintf_s(key, L"hold%d", i);
        if (!WriteNumber(key, binds::items[i].hold ? 1 : 0)) ok = false;
    }
    for (const auto& setting : floats) if (!WriteNumber(setting.key, (int)(*setting.target*100.f))) ok=false;
    wchar_t heroName[64]={};if(MultiByteToWideChar(CP_UTF8,0,cfg::ksHeroName,-1,heroName,64)>0)if(!WritePrivateProfileStringW(profileNames[profile],L"ksHeroName",heroName,ConfigPath()))ok=false;
    wchar_t nickname[64]={};
    if (MultiByteToWideChar(CP_UTF8,0,cfg::hudNickname,-1,nickname,64)>0)
        if (!WritePrivateProfileStringW(profileNames[profile],L"nickname",nickname,ConfigPath())) ok=false;
    combos::InitializeSettings();
    for(int i=0;i<combos::count;++i){
        wchar_t key[64];const auto& c=combos::settings[i];
        swprintf_s(key,L"combo%dEnabled",i);if(!WriteNumber(key,c.enabled?1:0))ok=false;
        swprintf_s(key,L"combo%dHold",i);if(!WriteNumber(key,c.holdKey))ok=false;
        for(int j=0;j<combos::profiles[i].count;++j){swprintf_s(key,L"combo%dKey%d",i,j);if(!WriteNumber(key,c.keys[j]))ok=false;}
    }
    if (ok) {
        wchar_t n[8]; swprintf_s(n, L"%d", profile);
        ok = WritePrivateProfileStringW(L"client", L"active", n, ConfigPath()) != FALSE;
    }
    return ok;
}
static bool Load() {
    const wchar_t* path = ConfigPath();
    if (!path[0] || GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return false;
    wchar_t available[4] = {};
    if (!GetPrivateProfileStringW(profileNames[profile], L"espHeroes", L"", available, 4, path)) return false;
    for (auto& b : booleans) *b.target = GetPrivateProfileIntW(profileNames[profile], b.key, *b.target ? 1 : 0, path) != 0;
    language = (int)GetPrivateProfileIntW(profileNames[profile], L"language", 0, path);
    if (language < 0 || language > 1) language = 0;
    accent = (int)GetPrivateProfileIntW(profileNames[profile], L"accent", 0, path);
    if (accent < 0 || accent > 2) accent = 0;
    int scale = (int)GetPrivateProfileIntW(profileNames[profile], L"scale", 100, path);
    if (scale < 90 || scale > 130) scale = 100;
    fontScale = scale / 100.f;
    for (int i = 0; i < binds::kCount; ++i) {
        wchar_t key[32]; swprintf_s(key, L"bind%d", i);
        int vk = (int)GetPrivateProfileIntW(profileNames[profile], key, 0, path);
        binds::items[i].vk = vk >= 0 && vk <= 255 ? vk : 0;
        swprintf_s(key, L"hold%d", i);
        binds::items[i].hold = GetPrivateProfileIntW(profileNames[profile], key, 0, path) != 0;
        binds::items[i].holding = false;
    }
    for(auto& setting : floats) {
        float value=(int)GetPrivateProfileIntW(profileNames[profile],setting.key,(int)(*setting.target*100.f),path)/100.f;
        if (value>=setting.lo && value<=setting.hi) *setting.target=value;
    }
    wchar_t nickname[64]={};
    GetPrivateProfileStringW(profileNames[profile],L"nickname",L"player",nickname,64,path);
    if (!WideCharToMultiByte(CP_UTF8,0,nickname,-1,cfg::hudNickname,(int)sizeof(cfg::hudNickname),nullptr,nullptr))
        strcpy_s(cfg::hudNickname,"player");
    if(GetPrivateProfileIntW(profileNames[profile],L"uiVersion",2,path)<4) {
        cfg::hudTopAbilities=true;
        cfg::hudSkillPixels=35.f;
    }
    if(GetPrivateProfileIntW(profileNames[profile],L"uiVersion",2,path)<11){
        cfg::hudHpNumber=true;cfg::hudItems=true;cfg::hudAbilities=true;cfg::hudStatusBadges=true;
        cfg::hudIllusions=true;cfg::showEffects=true;cfg::hudSkillPixels=30.f;
    }
    // User-requested ESP19 default: migrate old profiles once, then preserve future edits.
    if(GetPrivateProfileIntW(profileNames[profile],L"uiVersion",2,path)<12){
        cfg::nativeHpOffsetY=-9.f;
        WriteNumber(L"uiVersion",12);WriteNumber(L"nativeHpOffsetY",-900);
    }
    if(GetPrivateProfileIntW(profileNames[profile],L"uiVersion",2,path)<7){accent=0;fontScale=1.f;}
    wchar_t heroName[64]={};GetPrivateProfileStringW(profileNames[profile],L"ksHeroName",L"",heroName,64,path);
    if(!WideCharToMultiByte(CP_UTF8,0,heroName,-1,cfg::ksHeroName,64,nullptr,nullptr))cfg::ksHeroName[0]=0;
    cfg::ksAbilitySlot=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],L"ksSlot",3,path),0,15);
    cfg::ksSpellKey=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],L"ksSpellKey",0,path),0,255);
    cfg::ksDamageType=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],L"ksType",0,path),0,2);
    binds::items[10].hold=true;
    cfg::armletKey=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],L"armletKey",0,path),0,255);
    cfg::armletSlot=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],L"armletSlot",0,path),0,5);
    cfg::armletConfirmed=false;armlet::Reset(); // never persist a verified-input claim across startup/profile changes
    combos::InitializeSettings();combos::Reset();
    for(int i=0;i<combos::count;++i){
        wchar_t key[64];auto& c=combos::settings[i];c.confirmed=false;
        swprintf_s(key,L"combo%dEnabled",i);c.enabled=GetPrivateProfileIntW(profileNames[profile],key,0,path)!=0;
        swprintf_s(key,L"combo%dHold",i);c.holdKey=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],key,0,path),0,255);
        for(int j=0;j<combos::profiles[i].count;++j){swprintf_s(key,L"combo%dKey%d",i,j);c.keys[j]=std::clamp((int)GetPrivateProfileIntW(profileNames[profile],key,combos::profiles[i].steps[j].defaultKey,path),0,255);}
    }
    cfg::vbe = false; // legacy fog writes are never enabled by a visibility-information profile
    if(!binds::items[0].vk)binds::items[0].vk='V';
    binds::items[0].hold=true;
    return true;
}
static ImVec4 Accent(){
    return accent==1?ImVec4(.745f,.553f,1.f,1):accent==2?ImVec4(.35f,.82f,.67f,1):UI_RGB(32,157,212);
}
static void Style(){theme::ApplyStyle();auto c=Accent();auto& s=ImGui::GetStyle();
    s.Colors[ImGuiCol_CheckMark]=s.Colors[ImGuiCol_SliderGrab]=s.Colors[ImGuiCol_NavCursor]=c;
}
static void Help(const char* ru,const char* en){ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));ImGui::TextWrapped("%s",T(ru,en));ImGui::PopStyleColor();}
static void Row(const char* ru,const char* en,bool* target,int bind=-1){
    const char* label=T(ru,en);ImGui::PushID(label);
    ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x,h=36*std::max(1.f,fontScale);
    if(ImGui::InvisibleButton("toggle",ImVec2(w,h),ImGuiButtonFlags_EnableNav))*target=!*target;
    if(bind>=0&&ImGui::IsItemClicked(ImGuiMouseButton_Right))s_openReq=bind;
    auto* dl=ImGui::GetWindowDrawList();float cy=p.y+h*.5f;
    if(ImGui::IsItemHovered())dl->AddRectFilled(p,ImVec2(p.x+w,p.y+h),IM_COL32(27,24,33,255),5);
    uiicons::Draw(dl,ImVec2(p.x+8,cy),18,*target?ImGui::GetColorU32(Accent()):IM_COL32(102,108,118,255),uiicons::Check);
    float maxLabel=w-68;dl->PushClipRect(ImVec2(p.x+26,p.y),ImVec2(p.x+w-42,p.y+h),true);
    dl->AddText(ImVec2(p.x+26,cy-ImGui::GetFontSize()*.5f),ImGui::GetColorU32(ImGuiCol_Text),label);dl->PopClipRect();
    ImVec2 tr(p.x+w-33,cy-8);dl->AddRectFilled(tr,ImVec2(tr.x+32,tr.y+16),*target?ImGui::GetColorU32(Accent()):IM_COL32(55,59,66,255),8);
    dl->AddCircleFilled(ImVec2(tr.x+8+(*target?16:0),cy),6,IM_COL32(206,210,218,255));
    if(ImGui::IsItemHovered()){
        if(bind>=0)ImGui::SetTooltip("%s\n%s / RMB",label,binds::KeyName(binds::items[bind].vk));
        else if(ImGui::CalcTextSize(label).x>maxLabel)ImGui::SetTooltip("%s",label);
    }
    if(ImGui::IsItemFocused())dl->AddRect(p,ImVec2(p.x+w,p.y+h),ImGui::GetColorU32(Accent()),5);
    ImGui::PopID();
}
static bool LotusSlider(const char* label,float* value,float lo,float hi,const char* format){
    ImGui::PushID(label);float width=ImGui::CalcItemWidth();if(width<1)width=ImGui::GetContentRegionAvail().x;
    ImVec2 p=ImGui::GetCursorScreenPos();char text[40];snprintf(text,sizeof(text),format,*value);
    uiicons::Draw(ImGui::GetWindowDrawList(),ImVec2(p.x+8,p.y+ImGui::GetFontSize()*.5f),18,ImGui::GetColorU32(ImGuiCol_TextDisabled),uiicons::Sliders);
    if(label[0]!='#')ImGui::GetWindowDrawList()->AddText(ImVec2(p.x+26,p.y),ImGui::GetColorU32(ImGuiCol_TextDisabled),label);
    ImGui::Dummy(ImVec2(width,ImGui::GetFontSize()));
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x+width-ImGui::CalcTextSize(text).x,p.y),ImGui::GetColorU32(ImGuiCol_Text),text);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_SliderGrab,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0,6));ImGui::SetNextItemWidth(width);
    bool changed=ImGui::SliderFloat("##value",value,lo,hi,"",ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleVar();ImGui::PopStyleColor(6);
    ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();float cy=(a.y+b.y)*.5f;
    float x=a.x+8+(b.x-a.x-16)*((*value-lo)/(hi-lo));auto* dl=ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(a.x+2,cy-2),ImVec2(b.x-2,cy+2),IM_COL32(48,51,58,255),2);
    dl->AddRectFilled(ImVec2(a.x+2,cy-2),ImVec2(x,cy+2),ImGui::GetColorU32(Accent()),2);
    dl->AddCircleFilled(ImVec2(x,cy),5,IM_COL32(206,210,218,255));
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",text);
    ImGui::PopID();return changed;
}
static void Card(const char* id,const char* ru,const char* en,float height){
    ImGui::TextDisabled("%s  %s", "v",T(ru,en));ImGui::Dummy(ImVec2(0,3));
    const bool fitContent=height<0;
    if(fitContent)ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(16,8*std::max(1.f,fontScale)));
    ImGuiChildFlags flags=ImGuiChildFlags_Borders;
    if(fitContent)flags|=ImGuiChildFlags_AutoResizeY|ImGuiChildFlags_AlwaysAutoResize;
    ImGui::BeginChild(id,ImVec2(0,fitContent?0.f:(height>0?height*.92f*std::max(1.f,fontScale):-18.f)),flags);
    if(fitContent)ImGui::PopStyleVar();
    ImGui::SetWindowFontScale(fontScale);
}
static void EndCard(float gap=9){ImGui::EndChild();if(gap>0)ImGui::Dummy(ImVec2(0,gap));}
static void NavIcon(ImDrawList* dl,ImVec2 p,int type,ImU32 color) {
    static const uiicons::Icon icons[]={uiicons::Render,uiicons::Combat,uiicons::Map,uiicons::Grid,uiicons::Configs,uiicons::Settings,uiicons::Diagnostics,uiicons::Assist};
    if(type>=0&&type<8)uiicons::Draw(dl,p,23,color,icons[type]);
}
static bool Navigation(const char* name,int type,bool selected,bool rail=false){
    (void)rail;ImGui::PushID(type);ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x,h=40;
    bool clicked=ImGui::InvisibleButton("navigation",ImVec2(w,h),ImGuiButtonFlags_EnableNav);auto* dl=ImGui::GetWindowDrawList();
    ImU32 color=selected?ImGui::GetColorU32(Accent()):IM_COL32(160,154,172,255);
    if(selected||ImGui::IsItemHovered())dl->AddRectFilled(p,ImVec2(p.x+w,p.y+h),selected?IM_COL32(45,36,59,255):IM_COL32(31,28,37,255),6);
    if(selected)dl->AddRectFilled(ImVec2(p.x,p.y+12),ImVec2(p.x+3,p.y+h-12),color,2);
    NavIcon(dl,ImVec2(p.x+20,p.y+h*.5f),type,color);
    dl->PushClipRect(p,ImVec2(p.x+w,p.y+h),true);dl->AddText(ImVec2(p.x+40,p.y+(h-ImGui::GetFontSize())*.5f),color,name);dl->PopClipRect();
    if(ImGui::IsItemHovered()&&ImGui::CalcTextSize(name).x>w-44)ImGui::SetTooltip("%s",name);
    ImGui::PopID();return clicked;
}
static void Unsupported(const char* name,const char* ru,const char* en){
    ImGui::TextUnformatted(name);ImGui::SameLine();ImGui::TextDisabled("/ %s",T("не реализовано","not implemented"));Help(ru,en);ImGui::Spacing();
}
static void Diagnostics() {
        const auto& d = diagnostics::snapshot;
        const auto& p = game::g_probe;
        const char* why = "waiting for probe";
        switch (p.stage.load()) {
        case 1: why = "probe in progress"; break;
        case 2: why = "global slot unreadable"; break;
        case 3: why = "global value is not a valid pointer"; break;
        case 4: why = "object vtable unreadable"; break;
        case 5: why = "object vtable does not match dump"; break;
        case 6: why = "no valid entity-page pointers"; break;
        case 7: why = "entity identity checks rejected entries"; break;
        case 8: why = "entity discovery passed"; break;
        case 9: why = "searching writable module data"; break;
        case 10: why = "no object with expected vtable found"; break;
        case 11: why = "search limit reached (not whole module)"; break;
        case 12: why = "waiting for next scan (30 second interval)"; break;
        }
        ImGui::Text("Attempts: %u", p.attempts.load());
        ImGui::TextWrapped("Result: %s", why);
        ImGui::Text("Client: 0x%llX", (unsigned long long)game::g_sys.clientBase);
        ImGui::Text("Global slot: 0x%llX", (unsigned long long)p.slot.load());
        ImGui::Text("Global value: 0x%llX", (unsigned long long)p.pointer.load());
        ImGui::Text("Actual vtable: 0x%llX", (unsigned long long)p.vtable.load());
        ImGui::Text("Expected vtable: 0x%llX", (unsigned long long)p.expectedVtable.load());
        const char* cls = rtti::ClassOf(game::g_sys.clientBase, p.vtable.load());
        ImGui::TextWrapped("RTTI lookup: %s", cls ? cls : "not found");
        ImGui::Text("Object +08/+10/+18: 0x%llX / 0x%llX / 0x%llX", (unsigned long long)p.word08.load(), (unsigned long long)p.word10.load(), (unsigned long long)p.word18.load());
        ImGui::Text("Valid pages: %d  Validation hits (max 8): %d", p.pages.load(), p.hits.load());
        ImGui::Text("Slots scanned: %u  Vtable matches: %u", p.scannedSlots.load(), p.matchedObjects.load());
        ImGui::Text("Last scan code: %d", p.lastScanStage.load());
        ImGui::Separator();
        ImGui::Text("System: %s  Controller: %s", game::g_sys.ready ? "ready" : "not ready",
                    mem::ValidPtr(game::g_sys.localCtrl) ? "found" : "missing");
        ImGui::Text("Local frame: %s | World view: %s",d.frameOk?"valid":"unavailable",d.observedOnly?"observed-only":d.frameOk?"full":"unavailable");
        ImGui::Text("Units: %d | Heroes: %d | Projected units: %d",d.unitCount,d.heroCount,d.projectedUnits);
        ImGui::Text("Matrix: %s  ESP: %s", d.matrixOk ? "plausible" : "invalid", cfg::espHeroes ? "on" : "off");
        const auto& cp = game::g_controllerProbe;
        ImGui::TextUnformatted("ESP patch: ESP19 / base 40a11e81");
        ImGui::Text("Scanner passes: %u | faults: %u | exception: 0x%X",cp.scanPasses.load(),cp.scanFaults.load(),cp.exceptionCode.load());
        auto heartbeat = cp.scanHeartbeat.load();
        auto currentTick = GetTickCount64();
        ImGui::Text("Scanner age: %llu ms",(unsigned long long)(heartbeat && currentTick>=heartbeat ? currentTick-heartbeat : 0));
        ImGui::Text("Entities scanned: %d | controllers: %d | local flags: %d | source: %d",cp.scannedEntities.load(),cp.candidates.load(),cp.localFlags.load(),cp.source.load());
        ImGui::TextWrapped("Dump local-player global disabled: it pointed to C_DOTAGamerules in CTRL1.");
        ImGui::Text("Classified: %d units / %d heroes",cp.classifiedUnits.load(),cp.classifiedHeroes.load());
        ImGui::Text("Controller refs: stage=%d slots=%u hits=%d",cp.controllerRefStage.load(),cp.controllerRefSlots.load(),cp.controllerRefHits.load());
        if(d.observedOnly)ImGui::TextWrapped("Basic world ESP only. Local identity/team is unknown; automation remains blocked.");
        ImGui::Text("Frame gate: %d | hero handle 0x%X | hero 0x%llX",cp.frameStage.load(),cp.heroHandle.load(),(unsigned long long)cp.heroAddress.load());
        ImGui::Text("Projection mode: %d | sample hero position: %.1f %.1f %.1f",d.projectionMode,d.sampleWorld.x,d.sampleWorld.y,d.sampleWorld.z);
        for(int row=0;row<4;++row)ImGui::Text("M%d: %.5g %.5g %.5g %.5g",row,d.matrix[row*4],d.matrix[row*4+1],d.matrix[row*4+2],d.matrix[row*4+3]);
        ImGui::Text("Y votes: upright=%d flipped=%d | zero-origin heroes=%d",d.uprightVotes,d.flippedVotes,d.zeroOriginHeroes);
        if(ImGui::Button("Copy ESP diagnostics")) {
            char report[32768];snprintf(report,sizeof(report),
                "ESP19\nentities=%d classified=%d classifiedHeroes=%d controllers=%d localFlags=%d source=%d\nscannerPasses=%u faults=%u exception=0x%X frameGate=%d\nlocalFrame=%d observedOnly=%d units=%d heroes=%d projectedUnits=%d matrixPlausible=%d mode=%d\nsampleHero=%g,%g,%g\nM0=%g,%g,%g,%g\nM1=%g,%g,%g,%g\nM2=%g,%g,%g,%g\nM3=%g,%g,%g,%g\n",
                cp.scannedEntities.load(),cp.classifiedUnits.load(),cp.classifiedHeroes.load(),cp.candidates.load(),cp.localFlags.load(),cp.source.load(),
                cp.scanPasses.load(),cp.scanFaults.load(),cp.exceptionCode.load(),cp.frameStage.load(),d.frameOk,d.observedOnly,d.unitCount,d.heroCount,d.projectedUnits,d.matrixOk,d.projectionMode,
                d.sampleWorld.x,d.sampleWorld.y,d.sampleWorld.z,
                d.matrix[0],d.matrix[1],d.matrix[2],d.matrix[3],d.matrix[4],d.matrix[5],d.matrix[6],d.matrix[7],d.matrix[8],d.matrix[9],d.matrix[10],d.matrix[11],d.matrix[12],d.matrix[13],d.matrix[14],d.matrix[15]);
            size_t used=strlen(report);
            int added=snprintf(report+used,sizeof(report)-used,"Yvotes=%d,%d zeroOriginHeroes=%d screen=%d,%d refsStage=%d refsSlots=%u refsHits=%d\n",d.uprightVotes,d.flippedVotes,d.zeroOriginHeroes,view::W,view::H,cp.controllerRefStage.load(),cp.controllerRefSlots.load(),cp.controllerRefHits.load());
            if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);
            if(used<sizeof(report)-1){added=snprintf(report+used,sizeof(report)-used,"teamDataCandidates: Radiant=0x%llX Dire=0x%llX\n",(unsigned long long)game::g_teamVisibilityData[0].load(),(unsigned long long)game::g_teamVisibilityData[1].load());if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);}
            for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& p=d.samples[i];
                added=snprintf(report+used,sizeof(report)-used,"hero[%d]=%s pos=%g,%g,%g hp=%d alive=%d nodeRead=%d ownerMatch=%d footOK=%d foot=%g,%g headOK=%d head=%g,%g h=%g\n",i,p.name,p.pos.x,p.pos.y,p.pos.z,p.hp,p.alive,p.nodeRead,p.ownerMatches,p.footProjected,p.foot.x,p.foot.y,p.headProjected,p.head.x,p.head.y,p.foot.y-p.head.y);
                if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);
                if(used<sizeof(report)-1){added=snprintf(report+used,sizeof(report)-used,"data[%d]: level=%d inventoryRead=%d slots=%d resolved=%d unmapped=%d items=%d abilities=%d buffsRead=%d buffVisual=%d buffs=%d illusionRead=%d illusion=%d invisRead=%d stateRead=%d state=0x%llX clockRead=%d clock=%g team=%d visionRead=%d visionMask=0x%X forcedVBE=%d\n",i,p.level,p.inventoryRead,p.inventorySlots,p.resolved,p.unmapped,p.items,p.abilities,p.buffsRead,p.buffsVisualRead,p.buffs,p.illusionRead,p.illusion,p.invisRead,p.stateRead,(unsigned long long)p.state,p.clockRead,p.clock,p.team,p.visionRead,p.visionMask,cfg::vbe);if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);}
                if(used<sizeof(report)-1){added=snprintf(report+used,sizeof(report)-used,"npcVision[%d]: dataHandle=0x%X index=%u tableRead=%d visible=%d probeMask=%d word=0x%llX selfWord=0x%llX dormantRead=%d dormant=%d offsetsLiveVerified=0\n",i,p.npcDataHandle,p.npcIndex,p.npcVisionRead,p.npcVisible,p.npcProbeMask,(unsigned long long)p.npcWord,(unsigned long long)p.npcSelfWord,p.sceneDormantRead,p.sceneDormant);if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);}
            }
            for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& sample=d.samples[i];
                for(int j=0;j<sample.shownAbilityCount&&used<sizeof(report)-1;++j){added=snprintf(report+used,sizeof(report)-used,"ability[%d,%d]=%s level=%d max=%d\n",i,j,sample.abilityNames[j],sample.abilityLevels[j],sample.abilityMaxima[j]);if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);}}
            for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& sample=d.samples[i];
                for(int j=0;j<sample.shownBuffCount&&used<sizeof(report)-1;++j){added=snprintf(report+used,sizeof(report)-used,"modifier[%d,%d]=%s stacks=%d duration=%g expires=%g clock=%g\n",i,j,sample.buffNames[j],sample.buffStacks[j],sample.buffDuration[j],sample.buffExpires[j],sample.clock);if(added>0)used+=std::min(size_t(added),sizeof(report)-used-1);}}
            if(used<sizeof(report)-1)snprintf(report+used,sizeof(report)-used,"Armlet: %s\nArmlet last closed-menu status: %s\n",armlet::Status(),armlet::LastClosedStatus());
            ImGui::SetClipboardText(report);
        }
        ImGui::Text("HUD inventories: %d / %d heroes", hud::readProbe.inventories,hud::readProbe.heroes);
        ImGui::Text("Modifier lists: %d verified / %d unknown; %d effects",hud::readProbe.buffLists,hud::readProbe.buffUnknown,hud::readProbe.buffEffects);
        ImGui::Text("Items: %d (%d mapped)  Abilities: %d (%d mapped)", hud::readProbe.items,hud::readProbe.mappedItems,hud::readProbe.abilities,hud::readProbe.mappedAbilities);
        ImGui::Text("Aegis: %d detected, %d timed, %d estimated, %d owner fallback",hud::readProbe.aegis,hud::readProbe.aegisTimed,hud::readProbe.aegisEstimated,hud::readProbe.aegisFallback);
        ImGui::Text("Inventory slots: %d  Resolved: %d  Unmapped: %d",hud::readProbe.inventorySlots,hud::readProbe.resolved,hud::readProbe.unmapped);
        ImGui::TextUnformatted("ESP19 top HUD: compact horizontal rows / stable player slots.");
        ImGui::Text("Team-data candidates: Radiant=0x%llX Dire=0x%llX",(unsigned long long)game::g_teamVisibilityData[0].load(),(unsigned long long)game::g_teamVisibilityData[1].load());
        ImGui::TextWrapped("Inventory layout and cooldown direction need runtime verification.");

}
}
namespace clientui {
struct Function { const char* name;bool* enabled;int details;int bind; };
static Function renderFunctions[]={
    {"HeroInfo",&cfg::espHeroes,0,4},{"TopPanel",&cfg::hudTop,1,-1},
    {"IllusionDetector",&cfg::hudIllusions,2,-1},
    {"Watermark",&cfg::hudWatermark,4,-1},{"Keybinds",&cfg::showKeybinds,5,-1}};
static Function combatFunctions[]={
    {"Last-Hit",&cfg::lastHitEnabled,6,0},{"Auto-attack creeps",&cfg::farmAuto,7,1},
    {"Danger alerts",&cfg::dodger,8,-1},{"Click dodge",&cfg::autoDodge,9,8},{"Kill Stealer",&cfg::killStealer,19,10},{"Kill helper",&cfg::showKillHelper,21,-1}};
static Function mapFunctions[]={
    {"RoshanController",&cfg::hudRoshan,10,-1},{"Wards",&cfg::espWards,11,5},{"CreepBounty",&cfg::hudBounty,12,-1},{"Map notifications",nullptr,18,-1}};
static Function moreFunctions[]={ {"Skin Changer",nullptr,13,-1},{"Active Buffs / Debuffs",&cfg::showEffects,14,-1},{"Draft advisor",nullptr,23,-1}};
static Function profileFunctions[]={{"Local profiles",nullptr,15,-1}};
static Function interfaceFunctions[]={{"Interface",nullptr,16,-1},{"Auto-accept",&cfg::autoAccept,20,2}};
static Function diagnosticFunctions[]={{"Data diagnostics",nullptr,17,-1}};
static Function helperFunctions[combos::count+1];
static constexpr int heroCount=combos::count+1;
static void InitHelpers(){static bool done=false;if(done)return;done=true;combos::InitializeSettings();helperFunctions[0]={"Huskar / Armlet",&cfg::armletAuto,22,11};for(int i=0;i<combos::count;++i)helperFunctions[i+1]={combos::profiles[i].label,&combos::settings[i].enabled,2000+i,-1};}
static Function* functions[]={renderFunctions,combatFunctions,mapFunctions,moreFunctions,profileFunctions,interfaceFunctions,diagnosticFunctions,helperFunctions};
static const int functionCounts[]={5,6,4,3,1,2,1,heroCount};
static int category=0,selected=0;static char search[80]={};
static bool FunctionEntry(Function& fn,bool selected){
    ImGui::PushID(fn.details);ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
    float h=38*std::max(1.f,fontScale);bool clicked=ImGui::InvisibleButton("function",ImVec2(w,h),ImGuiButtonFlags_EnableNav);
    bool open=ImGui::IsItemClicked(ImGuiMouseButton_Right);if(clicked){if(fn.enabled)*fn.enabled=!*fn.enabled;else open=true;}
    auto* dl=ImGui::GetWindowDrawList();
    if(selected||ImGui::IsItemHovered())dl->AddRectFilled(p,ImVec2(p.x+w,p.y+h),selected?IM_COL32(25,29,34,255):IM_COL32(23,26,31,255),3);
    if(selected)dl->AddRectFilled(ImVec2(p.x,p.y+7),ImVec2(p.x+2,p.y+h-7),ImGui::GetColorU32(Accent()),1);
    dl->PushClipRect(p,ImVec2(p.x+w-7,p.y+h),true);dl->AddText(ImVec2(p.x+11,p.y+(h-ImGui::GetFontSize())*.5f),selected?ImGui::GetColorU32(ImGuiCol_Text):ImGui::GetColorU32(ImGuiCol_TextDisabled),fn.name);dl->PopClipRect();
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s\n%s",fn.name,T("ЛКМ: вкл/выкл · ПКМ: настройки","LMB: toggle · RMB: settings"));
    ImGui::PopID();return open;
}
static void MainSwitch(Function& fn){
    if(fn.enabled)Row("Включить","Enable",fn.enabled);
    if(fn.enabled&&fn.bind>=0){
        const float y=ImGui::GetItemRectMax().y+4.f;auto* dl=ImGui::GetWindowDrawList();
        const ImVec2 box=ImGui::GetWindowPos();
        dl->AddLine(ImVec2(box.x+1,y),ImVec2(box.x+ImGui::GetWindowSize().x-1,y),IM_COL32(32,34,40,255),1.f);
    }
    if(fn.bind>=0){
        ImGui::PushID(fn.bind+1000);ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x,h=36*std::max(1.f,fontScale);
        if(ImGui::InvisibleButton("binding",ImVec2(w,h),ImGuiButtonFlags_EnableNav))s_openReq=fn.bind;
        auto* dl=ImGui::GetWindowDrawList();float cy=p.y+h*.5f;
        uiicons::Draw(dl,ImVec2(p.x+8,cy),18,ImGui::GetColorU32(ImGuiCol_TextDisabled),uiicons::Keyboard);
        dl->AddText(ImVec2(p.x+26,cy-ImGui::GetFontSize()*.5f),ImGui::GetColorU32(ImGuiCol_TextDisabled),T("Клавиша","Key"));
        const char* key=binds::items[fn.bind].vk?binds::KeyName(binds::items[fn.bind].vk):"...";
        float bw=std::min(w*.45f,std::max(52.f,ImGui::CalcTextSize(key).x+22));
        ImVec2 badge(p.x+w-bw,p.y+(h-28)*.5f);
        dl->AddRectFilled(badge,ImVec2(p.x+w,badge.y+28),ImGui::IsItemHovered()?IM_COL32(35,40,48,255):IM_COL32(26,30,36,255),4);
        dl->PushClipRect(badge,ImVec2(p.x+w,badge.y+28),true);
        dl->AddText(ImVec2(badge.x+(bw-ImGui::CalcTextSize(key).x)*.5f,cy-ImGui::GetFontSize()*.5f),ImGui::GetColorU32(ImGuiCol_TextDisabled),key);dl->PopClipRect();
        if(ImGui::IsItemFocused())dl->AddRect(badge,ImVec2(p.x+w,badge.y+28),ImGui::GetColorU32(Accent()),4);
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s / %s",T("Сменить клавишу","Change key"),key);
        ImGui::PopID();
    }
}
}
void DrawMenuWindow(){
    using namespace clientui;if(!cfg::menuOpen)return;InitHelpers();
    if(!initialized){initialized=true;profile=ConfigPath()[0]?(int)GetPrivateProfileIntW(L"client",L"active",0,ConfigPath()):0;if(profile<0||profile>2)profile=0;Load();}
    Style();
    const char* catsRu[]={"Отображение","Бой","Карта","Прочее","Конфиги","Настройки","Диагностика","Прокасты / Помощник"};
    const char* catsEn[]={"Render","Combat","Map","Misc","Configs","Settings","Diagnostics","Combos / Helper"};
    ImVec2 screen=ImGui::GetIO().DisplaySize;float width=std::min(1040.f,std::max(600.f,screen.x-32)),height=std::min(730.f,std::max(440.f,screen.y-32));
    ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
    if(auto* old=ImGui::FindWindowByName("FDA Client")){
        ImVec2 clamped(std::max(16.f,std::min(old->Pos.x,std::max(16.f,screen.x-width-16))),std::max(16.f,std::min(old->Pos.y,std::max(16.f,screen.y-height-16))));
        if(clamped.x!=old->Pos.x||clamped.y!=old->Pos.y)ImGui::SetNextWindowPos(clamped,ImGuiCond_Always);
    }else ImGui::SetNextWindowPos(ImVec2((screen.x-width)*.5f,(screen.y-height)*.5f),ImGuiCond_FirstUseEver);
    ImGui::Begin("FDA Client",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetWindowFontScale(fontScale);ImVec2 p=ImGui::GetWindowPos(),sz=ImGui::GetWindowSize();auto* dl=ImGui::GetWindowDrawList();
    // Sampled reference palette: no live game texture and no alpha-composited backdrop.
    dl->AddRectFilled(p,ImVec2(p.x+sz.x,p.y+sz.y),IM_COL32(16,17,21,255),5);
    dl->AddRect(p,ImVec2(p.x+sz.x,p.y+sz.y),IM_COL32(24,26,31,255),5);
    float rail=52,listing=(width<850?182:208)*std::max(1.f,fontScale),detailX=rail+listing+28;
    dl->AddRectFilled(ImVec2(p.x+1,p.y+1),ImVec2(p.x+rail,p.y+sz.y-1),IM_COL32(19,20,24,255),5,ImDrawFlags_RoundCornersLeft);
    dl->AddRectFilled(ImVec2(p.x+rail,p.y+1),ImVec2(p.x+rail+listing+8,p.y+sz.y-1),IM_COL32(19,20,24,255));
    dl->AddLine(ImVec2(p.x+rail,p.y+12),ImVec2(p.x+rail,p.y+sz.y-12),IM_COL32(29,31,37,255),1.f);
    dl->AddLine(ImVec2(p.x+rail+listing+8,p.y+12),ImVec2(p.x+rail+listing+8,p.y+sz.y-12),IM_COL32(26,28,33,255),1.f);
    dl->AddLine(ImVec2(p.x+detailX-12,p.y+52),ImVec2(p.x+sz.x-18,p.y+52),IM_COL32(25,27,32,255),1.f);
    ImGui::SetCursorScreenPos(ImVec2(p.x+8,p.y+14));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0,0,0,0));ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::BeginChild("categories",ImVec2(rail-16,sz.y-28));ImGui::SetWindowFontScale(fontScale);
    for(int i=0;i<8;++i){ImGui::PushID(i+200);ImVec2 a=ImGui::GetCursorScreenPos();
        if(ImGui::InvisibleButton("category",ImVec2(36,44),ImGuiButtonFlags_EnableNav)){category=i;selected=0;search[0]=0;}
        if(category==i)ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(a.x-4,a.y+12),ImVec2(a.x-2,a.y+32),ImGui::GetColorU32(Accent()),1);
        NavIcon(ImGui::GetWindowDrawList(),ImVec2(a.x+18,a.y+22),i,category==i?ImGui::GetColorU32(Accent()):IM_COL32(117,123,135,255));
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",language?catsEn[i]:catsRu[i]);ImGui::PopID();
    }ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(p.x+rail+14,p.y+21));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0,0,0,0));ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::BeginChild("functions",ImVec2(listing-14,sz.y-48));ImGui::SetWindowFontScale(fontScale);
    ImGui::PushTextWrapPos(0);ImGui::TextDisabled("%s",language?catsEn[category]:catsRu[category]);ImGui::PopTextWrapPos();
    {ImVec2 line=ImGui::GetCursorScreenPos();ImGui::GetWindowDrawList()->AddLine(ImVec2(line.x,line.y+8),ImVec2(line.x+ImGui::GetContentRegionAvail().x,line.y+8),IM_COL32(27,29,34,255),1.f);}
    ImGui::Dummy(ImVec2(0,15));
    if(category==7){
        ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##hero-search",T("Поиск героя","Find hero"),search,sizeof(search));
        if(ImGui::Button(T("Показать доступные модули","Show implemented modules"),ImVec2(-1,28))){selected=0;search[0]=0;}
        ImGui::TextDisabled(T("%d подключённых модулей","%d implemented modules"),heroCount);ImGui::TextDisabled("%s",T("Базовые прокасты: экспериментально","Basic combos: experimental"));
        ImGui::BeginChild("hero-list",ImVec2(0,std::max(80.f,ImGui::GetContentRegionAvail().y-122)),ImGuiChildFlags_None);
    }
    bool found=false;
    for(int i=0;i<functionCounts[category];++i){Function& fn=functions[category][i];if(search[0]){std::string label=fn.name,needle=search;std::transform(label.begin(),label.end(),label.begin(),[](unsigned char c){return (char)std::tolower(c);});std::transform(needle.begin(),needle.end(),needle.begin(),[](unsigned char c){return (char)std::tolower(c);});if(label.find(needle)==std::string::npos)continue;}found=true;if(FunctionEntry(fn,selected==i))selected=i;}
    if(!found)ImGui::TextDisabled("%s",T("Ничего не найдено","No results"));
    if(category==7)ImGui::EndChild();
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY()+24,ImGui::GetWindowHeight()-96));
    ImGui::TextDisabled("FDA 0.5.5 / ESP19");ImGui::TextDisabled("%s",profileLabels[profile]);
    if(ImGui::Button(T("Сохранить","Save"),ImVec2(-1,30)))snprintf(message,sizeof(message),"%s",Save()?T("Сохранено","Saved"):T("Ошибка записи","Write error"));
    if(message[0])ImGui::TextWrapped("%s",message);
    ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();
    Function& fn=functions[category][selected];
    ImGui::SetCursorScreenPos(ImVec2(p.x+detailX,p.y+18));ImGui::TextDisabled("%s /",language?catsEn[category]:catsRu[category]);ImGui::SameLine();ImGui::TextColored(Accent(),"%s",fn.name);
    if(width>950){ImGui::SetCursorScreenPos(ImVec2(p.x+sz.x-256,p.y+13));ImGui::SetNextItemWidth(210);ImGui::InputTextWithHint("##function-search",T("Поиск функции","Find function"),search,sizeof(search));}
    ImGui::SetCursorScreenPos(ImVec2(p.x+sz.x-39,p.y+12));if(ImGui::Button("x",ImVec2(26,28)))cfg::menuOpen=false;
    ImGui::SetCursorScreenPos(ImVec2(p.x+detailX,p.y+66));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0,0,0,0));ImGui::BeginChild("details",ImVec2(sz.x-detailX-18,sz.y-98));ImGui::SetWindowFontScale(fontScale);ImGui::PopStyleColor();
    if((fn.details<15&&fn.details!=13)||fn.details==19||fn.details==20||fn.details==21||fn.details==22){Card("main","Основное","General",-1);MainSwitch(fn);EndCard(0);}
    Card("options","Настройки","Settings",0);
    if(fn.details>=2000&&fn.details<2000+combos::count)combos::DrawSettings(fn.details-2000,language!=0);
    switch(fn.details){
    case 0:
        Row("Способности и уровни","Abilities & levels",&cfg::hudAbilities);Row("Предметы и заряды","Items & charges",&cfg::hudItems);
        Row("Число HP","HP number",&cfg::hudHpNumber);
        Row("Статусы / INVIS","Statuses / INVIS",&cfg::hudStatusBadges);
        Help("Враг вне обзора: только круг с таймером 10 секунд. Возврат в обзор убирает круг. Нужен локальный игрок и подтверждённая командная NPC-таблица; нулевая маска модели не считается туманом. VBE должен быть выключен.","Enemy out of vision: a ten-second ring only. Reappearance cancels it. Requires a verified local frame and team NPC visibility table; zero model mask is not fog. Forced VBE must be disabled.");
        Row("Баффы и дебаффы","Buffs & debuffs",&cfg::showEffects);
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Сдвиг числа HP по X","Native HP number offset X"),&cfg::nativeHpOffsetX,-100,100,"%.0f px");
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Сдвиг числа HP по Y","Native HP number offset Y"),&cfg::nativeHpOffsetY,-100,100,"%.0f px");
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Доп. сдвиг HP красного бара (вниз +)","Red HP bar additional Y offset (+ down)"),&cfg::nativeHpRedOffsetY,-100,100,"%.0f px");
        if(ImGui::Button(T("Сбросить сдвиги HP","Reset HP offsets"))){cfg::nativeHpOffsetX=0;cfg::nativeHpOffsetY=-9;cfg::nativeHpRedOffsetY=6;}
        Row("Visible By Enemy *","Visible By Enemy *",&cfg::hudVisibleByEnemy,6);
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Размер иконок","Icon size"),&cfg::hudSkillPixels,28,60,"%.0f px");break;
    case 1:Row("Способности сверху","Top abilities",&cfg::hudTopAbilities);Row("Портреты героев","Hero portraits",&cfg::hudPortraits);ImGui::SetNextItemWidth(-1);LotusSlider(T("Высота компактной панели","Compact panel Y"),&cfg::hudTopY,54,240,"%.0f px");Help("Горизонтальные карточки: HP/MP, ряд способностей, ряд предметов. В тумане нет текущих HP/MP/спеллов. Малое разрешение: +N означает дополнительные скрытые иконки, а не вертикальный столбец.","Horizontal cards: HP/MP, ability row, item row. No current HP/MP/spells in fog. On smaller screens, +N counts extra icons rather than growing a vertical column.");break;
    case 2:Help("Показывает ILLUSION по прочитанному флагу или модификатору. Не угадывает по одинаковому герою.","Shows ILLUSION from verified flags/modifiers. Duplicate heroes alone are not proof.");break;
    case 4:ImGui::SetNextItemWidth(220);ImGui::InputText(T("Подпись","Label"),cfg::hudNickname,sizeof(cfg::hudNickname));break;
    case 5:Help("Показывает существующие привязки функций. Их можно менять кнопкой «Клавиша» в настройках функции.","Shows existing function bindings. Edit a binding using the Key button in the function settings.");break;
    case 6:Row("Минимальный урон вместо среднего","Minimum damage instead of average",&cfg::lastHitConservative);Row("Денай при смертельном HP","Deny at lethal HP",&cfg::deny);Row("Харрас после крипов","Harass after creeps",&cfg::farmHarass);Row("Метки добивания","Last-hit markers",&cfg::lastHit);
        Help("Режим удержания. Нижняя оценка физического урона с бронёй; неизвестные данные блокируют добивание. Время полёта и модификаторы не учтены — гарантии 1 HP нет.","Hold mode. Lower physical damage estimate with armor; unknown data blocks last-hit. Projectile time and modifiers are not modeled; no 1 HP guarantee.");break;
    case 7:Help("Автоатака крипов использует существующий алгоритм проекта.","Creep auto-attacks use the existing project algorithm.");break;
    case 8:Help("Только существующие предупреждения об опасности.","Existing danger alerts only.");break;
    case 9:Help("Уклонение кликами использует существующий алгоритм. Новые возможности этим меню не добавляются.","Click dodge uses the existing algorithm. This menu adds no new game capability.");break;
    case 10:Row("Рошан в мире","World Roshan",&cfg::espRoshan);Help("Панель Рошана перемещается при открытом меню.","Drag the Roshan panel while the menu is open.");
        if(ImGui::Button(T("Сбросить позицию","Reset position"))){cfg::hudRoshanX=-1;cfg::hudRoshanY=220;}break;
    case 11:Help("Только варды, доступные игровому клиенту. Скрытые серверные данные не восстанавливаются.","Only wards available to the client. Hidden server data is not recovered.");break;
    case 12:Help("Награда под курсором; случайная награда показывается диапазоном.","Cursor bounty; randomized bounty is shown as a range.");break;
    case 13:{
        Help("Экспериментальная локальная косметика. Видимость в матче ещё не проверена; предметы на аккаунт не добавляются.","Experimental local cosmetics. In-match visuals are not yet verified; no items are added to your account.");
        auto status=skins::GetStatus();bool on=status.enabled;ImGui::BeginDisabled(!status.connected);
        Row("Косметика в родном арсенале","Cosmetics in native armory",&on);ImGui::EndDisabled();
        if(on!=status.enabled)skins::SetEnabled(on);
        skins::DrawSettings(language!=0,fontScale);break;}

    case 14:Row("Только временные","Timed effects only",&cfg::effectsTimedOnly);
        Help("Flask / Tango / Clarity и другие прочитанные модификаторы: имя, остаток времени, стаки. Эффекты отображаются возле каждого героя с прочитанными данными.","Flask / Tango / Clarity and other readable modifiers: name, remaining time, stacks. Effects are shown inline for every hero with readable data.");
        Help("Экспериментальный поиск списка с проверкой каждого владельца. Проверенный положительный снимок можно показать; неполный список не разрешает автонажатия.","Experimental list discovery validates every owner. A validated positive snapshot can be displayed; an incomplete list never enables automation.");break;
    case 15:ImGui::SetNextItemWidth(200);ImGui::Combo(T("Профиль","Profile"),&profile,profileLabels,3);
        if(ImGui::Button(T("Сохранить","Save")))snprintf(message,sizeof(message),"%s",Save()?T("Сохранено","Saved"):T("Ошибка записи","Write error"));
        ImGui::SameLine();if(ImGui::Button(T("Загрузить","Load"))){bool ok=Load();if(ok){wchar_t n[8];swprintf_s(n,L"%d",profile);WritePrivateProfileStringW(L"client",L"active",n,ConfigPath());}snprintf(message,sizeof(message),"%s",ok?T("Загружено","Loaded"):T("Профиль отсутствует","Profile unavailable"));}
        if(message[0])ImGui::TextUnformatted(message);break;
    case 16:{const char* langs[]={"Русский","English"};const char* colors[]={"Blue","Lotus Violet","Mint"};ImGui::SetNextItemWidth(200);ImGui::Combo(T("Язык","Language"),&language,langs,2);
        ImGui::SetNextItemWidth(200);ImGui::Combo(T("Акцент","Accent"),&accent,colors,3);ImGui::SetNextItemWidth(240);LotusSlider(T("Масштаб текста","Text scale"),&fontScale,.9f,1.3f,"%.2f");
        Help("Непрозрачная тема по последнему образцу. Blur и просвечивание игры отключены.","Opaque theme from the latest reference. Blur and game transparency are disabled.");break;}
    case 19:{
        Row("Показывать Kill helper (без нажатий)","Show Kill helper (no inputs)",&cfg::showKillHelper);
        Help("Хелпер работает независимо от автонажатий. Его расчёт и фильтр — в разделе Kill helper.","The helper is independent of autocasting. Calculation and filter settings are under Kill helper.");
        if(ImGui::Button(T("Привязать профиль к текущему герою","Bind profile to current hero")))killstealer::BindCurrentHero();
        ImGui::TextWrapped("%s",cfg::ksHeroName[0]?cfg::ksHeroName:T("Герой не привязан","No hero bound"));
        Row("Quickcast на нажатие подтверждён","Quickcast-on-keydown confirmed",&cfg::ksQuickcastConfirmed);
        ImGui::SetNextItemWidth(180);if(ImGui::SliderInt(T("Слот способности (0..15)","Ability slot (0..15)"),&cfg::ksAbilitySlot,0,15))cfg::ksQuickcastConfirmed=false;
        static const char* keys[]={"Не задана / unset","Q","W","E","R","D","F","1","2","3","4","5","6"};static const int codes[]={0,'Q','W','E','R','D','F','1','2','3','4','5','6'};
        int selectedKey=0;for(int i=0;i<13;++i)if(codes[i]==cfg::ksSpellKey)selectedKey=i;
        ImGui::SetNextItemWidth(180);if(ImGui::Combo(T("Клавиша спелла","Spell key"),&selectedKey,keys,13)){cfg::ksSpellKey=codes[selectedKey];cfg::ksQuickcastConfirmed=false;}
        ImGui::SetNextItemWidth(180);ImGui::InputFloat(T("Урон до сопротивления","Raw spell damage"),&cfg::ksDamage,10,50,"%.0f");
        const char* types[]={"Magical","Physical","Pure"};ImGui::SetNextItemWidth(180);ImGui::Combo(T("Тип урона","Damage type"),&cfg::ksDamageType,types,3);
        ImGui::SetNextItemWidth(180);ImGui::InputFloat(T("Дальность","Cast range"),&cfg::ksRange,25,100,"%.0f");
        ImGui::SetNextItemWidth(180);ImGui::InputFloat(T("Запас HP","HP margin"),&cfg::ksMargin,5,20,"%.0f");
        Help("Ручной профиль одного спелла. Урон и дальность задай по своему текущему герою, уровню и талантам. При смене героя автоматизация блокируется до новой привязки.","Manual one-spell profile. Set damage/range for your current hero, level and talents. Hero changes block automation until rebound.");
        Help("Действует только при удержании бинда, закрытом меню и фокусе Dota. Нужны подтверждённые баффы, видимость, мана и готовый спелл. Не гарантирует убийство; каналы и снаряды не моделируются.","Requires hold binding, closed menu and Dota focus. Verified buffs, visibility, mana and ready spell are required. No kill guarantee; channels and projectiles are not modeled.");break;}
    case 23:{static std::array<int,4> picks={-1,-1,-1,-1};draftadvisor::DrawUI(language!=0,picks);break;}
    case 22:{
        Help("Huskar — экспериментальный Armlet по порогу HP. Не универсальный прокаст и не предсказатель входящего урона.","Huskar — experimental HP-threshold Armlet. Not a combo script or incoming-hit predictor.");
        int slot=cfg::armletSlot+1;ImGui::SetNextItemWidth(180);if(ImGui::SliderInt(T("Слот Armlet (1..6)","Armlet slot (1..6)"),&slot,1,6)){cfg::armletSlot=slot-1;cfg::armletConfirmed=false;armlet::Reset();}
        static const char* keys[]={"Не задана / unset","Q","W","E","R","D","F","Z","X","C","V","B","N","1","2","3","4","5","6"};static const int codes[]={0,'Q','W','E','R','D','F','Z','X','C','V','B','N','1','2','3','4','5','6'};
        int selectedKey=0;for(int i=0;i<IM_ARRAYSIZE(codes);++i)if(codes[i]==cfg::armletKey)selectedKey=i;
        ImGui::SetNextItemWidth(180);if(ImGui::Combo(T("Клавиша предмета в Dota","Native item key"),&selectedKey,keys,IM_ARRAYSIZE(keys))){cfg::armletKey=codes[selectedKey];cfg::armletConfirmed=false;armlet::Reset();}
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Переключать при HP <=","Toggle when HP <="),&cfg::armletThreshold,50,550,"%.0f HP");
        Row("Клавиша проверена в демо; риск понятен","Key tested in demo; risk understood",&cfg::armletConfirmed);
        Help("Клавиша сверху — необязательный бинд включения помощника (toggle или hold). Можно включить переключателем меню. Клавиша предмета — настоящая привязка выбранного слота в Dota. Автоматика работает только на своём Huskar, выбранном единственным юнитом, при закрытом меню и фокусе Dota.","The optional top binding activates the helper (toggle or hold); the menu switch also works. Native item key must match this inventory slot in Dota. Own Huskar must be the only selected unit, with menu closed and Dota focused.");
        Help("Настройка: 1) свой Huskar в демо; 2) Armlet в активном слоте 1–6; 3) указать настоящую клавишу этого слота и проверить её вручную; 4) включить подтверждение и модуль; 5) выбрать только Huskar, закрыть меню, вернуть фокус Dota. Отдельный бинд модуля необязателен. Порог HP должен быть ниже текущего HP для режима ожидания.","Setup: own Huskar in demo; Armlet in active slot 1–6; correct native slot key, manually tested; confirm and enable; select only Huskar, close menu, focus Dota. Activation binding is optional. Start above the HP threshold.");
        Help("При низком HP: если Armlet выключен — включить; если включён — выключить, дождаться подтверждённого OFF и затем включить. Не чаще 2 секунд, с подтверждением ON и активного модификатора. Неизвестные данные блокируют ввод. Распознанный риск DoT запрещает новый OFF, но не отменяет ON после уже запрошенного OFF. Урон и время тиков не рассчитываются.","Low HP: turn ON if OFF; if ON, request OFF, observe actual OFF and then request ON. Two-second minimum spacing and confirmed ON plus active modifier. Unknown data blocks input. Recognized DoT risk blocks a new OFF, but not ON completion after an OFF request. Damage and tick timing are not calculated.");
        Help("Входящие атаки, снаряды и все DoT НЕ предсказываются. Возможна смерть. При сбое или потере фокуса цикл останавливается: Armlet может остаться выключенным — проверь вручную. Модуль владеет input lane на своём Huskar или при незавершённом цикле; на другом герое его базовый прокаст не блокируется.","Incoming attacks/projectiles and every DoT are NOT predicted. Death is possible. Failure/focus loss stops the cycle: Armlet may remain OFF; check manually. Armlet owns the input lane on own Huskar or an unfinished cycle; another hero basic combo is not blocked by enabling this module.");
        if(!diagnostics::snapshot.frameOk)Help("БЛОК: локальный игрок не определён. Контроллер ищется отдельно; чужой Huskar не выбирается автоматически.","BLOCKED: local player unresolved. Reference search runs separately; no arbitrary Huskar selection.");
        if(!cfg::armletConfirmed)Help("БЛОК: подтверждение проверенной клавиши выключено.","BLOCKED: native-key confirmation is OFF.");
        if(ImGui::Button(T("Копировать диагностику Armlet","Copy Armlet diagnostics"))){
            char report[32768];const auto& d=diagnostics::snapshot;const auto& cp=game::g_controllerProbe;
            snprintf(report,sizeof(report),"ESP19 Armlet\ncurrent=%s\nlastClosedMenu=%s\nenabled=%d confirmed=%d nativeKey=%d slot=%d threshold=%g activationKey=%d menuOpen=%d\nlocalFrame=%d observedOnly=%d controller=%llX source=%d refsStage=%d refsSlots=%u refsHits=%d\n",armlet::Status(),armlet::LastClosedStatus(),cfg::armletAuto,cfg::armletConfirmed,cfg::armletKey,cfg::armletSlot+1,cfg::armletThreshold,binds::items[11].vk,cfg::menuOpen,d.frameOk,d.observedOnly,(unsigned long long)game::g_sys.localCtrl,cp.source.load(),cp.controllerRefStage.load(),cp.controllerRefSlots.load(),cp.controllerRefHits.load());
            uint32_t assigned=0;int player=-1,owner=-1,heroPlayer=-1;uint8_t localFlag=2;
            bool assignedRead=mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned);
            bool playerRead=mem::Read(game::g_sys.localCtrl+off::Ctrl::m_nPlayerID,player);
            bool flagRead=mem::Read(game::g_sys.localCtrl+off::Ctrl::m_bIsLocalPlayerController,localFlag);
            uintptr_t boundHero=assignedRead?game::EntityByHandle(assigned):0;
            bool ownerRead=boundHero&&mem::Read(boundHero+off::NPC::m_nPlayerOwnerID,owner);
            bool heroPlayerRead=boundHero&&mem::Read(boundHero+off::Hero::m_iPlayerID,heroPlayer);
            size_t bindLen=strlen(report);snprintf(report+bindLen,sizeof(report)-bindLen,"bind: frameGate=%d mask=0x%X assignedRead=%d assigned=0x%X hero=0x%llX playerRead=%d player=%d localFlagRead=%d localFlag=%d ownerRead=%d owner=%d heroPlayerRead=%d heroPlayer=%d\n",cp.frameStage.load(),game::g_sys.handleMask,assignedRead,assigned,(unsigned long long)boundHero,playerRead,player,flagRead,(int)localFlag,ownerRead,owner,heroPlayerRead,heroPlayer);
            size_t vlen=strlen(report);snprintf(report+vlen,sizeof(report)-vlen,"teamDataCandidates: Radiant=0x%llX Dire=0x%llX\n",(unsigned long long)game::g_teamVisibilityData[0].load(),(unsigned long long)game::g_teamVisibilityData[1].load());
            size_t infoLen=strlen(report);snprintf(report+infoLen,sizeof(report)-infoLen,"RTTI runtime attempts=%u parsed=%u\n",rtti::RuntimeAttempts(),rtti::RuntimeParsed());
            size_t used=strlen(report);for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& p=d.samples[i];int n=snprintf(report+used,sizeof(report)-used,"hero[%d]=%s hp=%d inventoryLayout=%d inventoryRead=%d buffsRead=%d buffVisual=%d items=%d buffs=%d clockRead=%d state=0x%llX\n",i,p.name,p.hp,p.inventoryLayout,p.inventoryRead,p.buffsRead,p.buffsVisualRead,p.items,p.buffs,p.clockRead,(unsigned long long)p.state);if(n>0)used+=std::min(size_t(n),sizeof(report)-used-1);}
            for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& p=d.samples[i];int n=snprintf(report+used,sizeof(report)-used,"inventory[%d]: layout=%d probeMask=%d raw0=0x%llX raw8=0x%llX parent=0x%X\n",i,p.inventoryLayout,p.inventoryProbeMask,(unsigned long long)p.inventoryRaw0,(unsigned long long)p.inventoryRaw8,p.inventoryParent);if(n>0)used+=std::min(size_t(n),sizeof(report)-used-1);}
            for(int i=0;i<d.samplesCount&&used<sizeof(report)-1;++i){const auto& p=d.samples[i];int n=snprintf(report+used,sizeof(report)-used,"identity[%d]: handle=0x%X ownerRead=%d owner=%d heroPlayerRead=%d heroPlayer=%d\n",i,p.entityHandle,p.ownerIDRead,p.ownerID,p.heroPlayerIDRead,p.heroPlayerID);if(n>0)used+=std::min(size_t(n),sizeof(report)-used-1);}
            if(used<sizeof(report)-1)snprintf(report+used,sizeof(report)-used,"%s%s%s%s%s",armlet::TraceDiagnostics(),armlet::SelectionDiagnostics(),armlet::DangerDiagnostics(),armlet::CycleDiagnostics(),armlet::DamageDiagnostics());
            ImGui::SetClipboardText(report);
        }
        Help("Закройте меню на 2–3 секунды, затем откройте и скопируйте диагностику. lastClosedMenu хранит последний статус при закрытом меню; иначе открытое меню само становится причиной блокировки.","Close menu for 2–3 seconds, reopen, then copy diagnostics. lastClosedMenu preserves the last status while the menu was closed.");
        ImGui::TextWrapped("%s",T(armlet::StatusRu(),armlet::Status()));if(ImGui::Button(T("Сбросить остановленный цикл","Reset stopped cycle"))){cfg::armletConfirmed=false;armlet::Reset();}break;}
    case 21:
        Row("Только изученные с моделью","Learned modeled skills only",&cfg::helperModeledOnly);
        ImGui::SetNextItemWidth(180);ImGui::InputFloat(T("Запас HP хелпера","Helper HP margin"),&cfg::helperMargin,5,20,"%.0f");
        cfg::helperMargin=std::isfinite(cfg::helperMargin)?std::clamp(cfg::helperMargin,0.f,1000.f):30.f;
        Help("Наведи курсор на видимого вражеского героя и закрой меню. Панель: способность, уровень, урон после сопротивления, урон минус HP, готовность и базовая дальность. Хелпер не нажимает клавиши и не выбирает врага сам.","Hover a visible enemy hero and close the menu. The panel shows skill, level, damage after resistance, damage minus HP, readiness and baseline range. No inputs or automatic target selection.");
        Help("17 базовых моделей: Lina, Lion, Zeus, Luna, Sven, Ogre, Pudge, Queen of Pain, Vengeful Spirit. Неподдерживаемый спелл можно оценить через ручной профиль Kill Stealer, привязанный к герою и слоту — автонажатия включать не нужно.","17 baseline models: Lina, Lion, Zeus, Luna, Sven, Ogre, Pudge, Queen of Pain, Vengeful Spirit. An unsupported spell can use the hero/slot-bound manual Kill Stealer profile; autocasting need not be enabled.");
        Help("Снимок базового урона одного попадания, не окончательный урон текущего клиента. Таланты, аспекты, Scepter/Shard, усиление, барьеры, стаки Finger, multicast, регенерация и попадание не моделируются. При неизвестной защите нет рекомендации добивать. По базе хватает — условная оценка, не гарантия.","Baseline single-impact snapshot, not current-client final damage. Talents, facets, Scepter/Shard, amplification, barriers, Finger stacks, multicast, regeneration and landing the hit are not modeled. Unknown protection prevents a finishing recommendation. BASE enough is conditional, not a guarantee.");break;
    case 20:Help("Один раз: включи, закрой меню, при настоящем приглашении наведи мышь на кнопку «Принять» и нажми F8. Этот матч прими вручную.","One-time setup: enable, close menu, hover the native Accept button during a real invite and press F8. Accept that first match manually.");
        Help("Следующие приглашения: свежий GC ReadyUpStatus + три совпадения образца. Dota должна быть в фокусе. При смене разрешения или внешнего вида нужна новая калибровка.","Next invites require fresh GC ReadyUpStatus plus three matching captures. Dota must have focus. Recalibrate after resolution or appearance changes.");
        {auto text=autoaccept::Status();ImGui::TextWrapped("%s",text.c_str());}
        if(ImGui::Button(T("Удалить образец кнопки","Remove button sample")))autoaccept::ClearTemplate();
        Help("Экспериментально: GDI-захват и настоящее принятие здесь не проверены. Несовпадение или неподтверждённый статус — без клика.","Experimental: GDI capture and actual acceptance are not tested here. A mismatch or unverified status means no click.");break;
    case 17:Diagnostics();break;
    case 18:
        Row("Руны: события клиента","Runes: client events",&cfg::notifyRunes);
        Row("До следующего появления","Upcoming spawn reminder",&cfg::notifyRuneSoon);
        Row("Обнаружен вражеский вард","Enemy ward discovered",&cfg::notifyWards);
        Row("Рошан: изменение HP","Roshan: health state change",&cfg::notifyRoshan);
        ImGui::SetNextItemWidth(-1);LotusSlider(T("Предупредить за","Reminder lead"),&cfg::notifyLead,3,30,"%.0f s");
        Help("Спавн — изменение времени в сущности спавнера. Обнаружение — руна стала доступна клиенту. Напоминание не подтверждает появление.","Spawn means the client spawner time changed. Discovery means a rune became available to the client. A reminder does not confirm a spawn.");
        Help("Только прочитанные сущности. Без восстановления тумана войны. При включении и смене матча старые события не повторяются.","Read entities only. No fog-of-war recovery. Old events are suppressed on enable and match changes.");break;
    }EndCard();ImGui::EndChild();
    if(s_openReq>=0){cfg::rebindIdx=s_openReq;s_capture=true;s_openReq=-1;ImGui::OpenPopup("rebind");}
    if(cfg::rebindIdx>=0){ImGui::SetNextWindowPos(ImVec2(std::max(16.f,std::min(p.x+sz.x-260,screen.x-260)),std::max(16.f,std::min(p.y+168,screen.y-210))),ImGuiCond_Appearing);if(ImGui::BeginPopup("rebind")){RebindPopup(cfg::rebindIdx);ImGui::EndPopup();}else{cfg::rebindIdx=-1;s_capture=false;}}
    ImGui::SetCursorScreenPos(ImVec2(p.x+detailX,p.y+sz.y-ImGui::GetFontSize()-18));ImGui::TextDisabled("%s",T("ЛКМ — вкл/выкл · ПКМ — настройки","LMB — toggle · RMB — settings"));
    ImGui::SetWindowFontScale(1);ImGui::End();
}
