#include "umbrella/menu.h"
#include "hud.h"
#include "hook.h"
#include "top_anchor.h"
#include "theme.h"
#include "aegis_tracker.h"
#include "hero_info.h"
#include "fog_memory.h"
#include "local_visibility.h"
#include "compact_top.h"
#include "world_clip.h"
#include "umbrella_style.h"
#include "imgui.h"
#include <wincodec.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <cmath>

namespace hud {
ReadProbe readProbe;
static int g_fogTeam=0;static uintptr_t g_fogRules=0;static uint32_t g_fogOwner=0;
static compacttop::Roster g_topRoster;
static fogmemory::Tracker g_fogMemory;
static std::vector<fogmemory::Marker> g_fogMarkers;
static heroinfo::InvisTracker g_invisTracker;
static heroinfo::Layout g_worldLayout;
static int g_worldLayoutFrame=-1;

static aegis::Tracker g_aegisTracker;
static ID3D11Device* g_device = nullptr;
static IWICImagingFactory* g_wic = nullptr;
static bool g_comOwned = false;
static DWORD g_comThread = 0;
static std::wstring g_root;
static std::unordered_map<std::string, ID3D11ShaderResourceView*> g_icons;

static std::wstring OwnDirectory() {
    wchar_t path[MAX_PATH] = {};
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&OwnDirectory), &module)) return {};
    DWORD n = GetModuleFileNameW(module,path,MAX_PATH);
    if (!n || n >= MAX_PATH) return {};
    std::wstring result(path);
    size_t at=result.find_last_of(L"\\/");
    return at==std::wstring::npos ? std::wstring{} : result.substr(0,at+1);
}
void Initialize(ID3D11Device* device) {
    Shutdown();
    if (!device) return;
    g_device=device; g_device->AddRef();
    HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    g_comOwned=SUCCEEDED(com);
    g_comThread=GetCurrentThreadId();
    CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g_wic));
    std::wstring dir=OwnDirectory();
    if (dir.empty()) return;
    g_root=dir+L"assets\\";
    if (GetFileAttributesW(g_root.c_str())==INVALID_FILE_ATTRIBUTES) g_root=dir+L"..\\assets\\";
    topanchor::Initialize(device);
}
void Shutdown() {
    g_fogMemory.Reset();g_fogMarkers.clear();g_topRoster.Reset();g_invisTracker.Reset();g_worldLayout.occupied.clear();g_worldLayoutFrame=-1;
    g_aegisTracker.Reset();
    topanchor::Shutdown();
    for (auto& pair:g_icons) if (pair.second) pair.second->Release();
    g_icons.clear();
    if (g_wic) {g_wic->Release();g_wic=nullptr;}
    if (g_device) {g_device->Release();g_device=nullptr;}
    if (g_comOwned && g_comThread==GetCurrentThreadId()) CoUninitialize();
    g_comOwned=false;g_comThread=0; // Do not uninitialize another thread's COM apartment.
    g_root.clear();
}
static ID3D11ShaderResourceView* Texture(const char* category,const char* name) {
    if (!g_device || !g_wic || !name || !*name || g_root.empty()) return nullptr;
    std::string key=std::string(category)+"/"+name;
    auto found=g_icons.find(key); if(found!=g_icons.end()) return found->second;
    // Accept only the generated ASCII icon identifiers; never arbitrary paths.
    for (const char* c=name;*c;++c)
        if (!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||*c=='_')) return nullptr;
    std::wstring file=g_root+std::wstring(category,category+strlen(category))+L"\\"+
                      std::wstring(name,name+strlen(name))+L".png";
    IWICBitmapDecoder* decoder=nullptr;
    IWICBitmapFrameDecode* frame=nullptr;
    IWICFormatConverter* converter=nullptr;
    ID3D11Texture2D* texture=nullptr;
    ID3D11ShaderResourceView* view=nullptr;
    UINT width=0,height=0;
    HRESULT hr=g_wic->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder);
    if(SUCCEEDED(hr)) hr=decoder->GetFrame(0,&frame);
    if(SUCCEEDED(hr)) hr=frame->GetSize(&width,&height);
    if(SUCCEEDED(hr) && (!width||!height||width>1024||height>1024)) hr=E_INVALIDARG;
    if(SUCCEEDED(hr)) hr=g_wic->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr)) hr=converter->Initialize(frame,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0.f,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)) {
        std::vector<unsigned char> pixels((size_t)width*height*4);
        hr=converter->CopyPixels(nullptr,width*4,(UINT)pixels.size(),pixels.data());
        if(SUCCEEDED(hr)) {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width=width;desc.Height=height;desc.MipLevels=1;desc.ArraySize=1;
            desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
            desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA initial{pixels.data(),width*4,0};
            hr=g_device->CreateTexture2D(&desc,&initial,&texture);
            if(SUCCEEDED(hr)) hr=g_device->CreateShaderResourceView(texture,nullptr,&view);
        }
    }
    if(texture)texture->Release();if(converter)converter->Release();if(frame)frame->Release();if(decoder)decoder->Release();
    if(g_icons.size()<2048)g_icons.emplace(key,view);
    else {if(view)view->Release();view=nullptr;}
    return view;
}
static float Clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
static void Text(ImDrawList* dl,ImVec2 p,const char* value,ImU32 color=IM_COL32_WHITE,float size=15.f,bool center=false,bool bold=false) {
    ImFont* font=bold?theme::FontBig():theme::FontSmall();
    if(!font)font=ImGui::GetFont();
    if(center)p.x-=font->CalcTextSizeA(size,10000,0,value).x*.5f;
    const ImU32 black=IM_COL32(0,0,0,235);
    dl->AddText(font,size,ImVec2(p.x-1,p.y),black,value);
    dl->AddText(font,size,ImVec2(p.x+1,p.y),black,value);
    dl->AddText(font,size,ImVec2(p.x,p.y-1),black,value);
    dl->AddText(font,size,ImVec2(p.x,p.y+1),black,value);
    dl->AddText(font,size,p,color,value);
}
static void Time(float seconds,char* out,size_t cap) {
    if(!std::isfinite(seconds)||seconds<0.f){snprintf(out,cap,"?");return;}
    int n=(int)ceilf(seconds);snprintf(out,cap,"%d:%02d",n/60,n%60);
}
static void Panel(ImDrawList* dl,ImVec2 p,ImVec2 size) {
    umbrellastyle::Halo(dl,p,{p.x+size.x,p.y+size.y},IM_COL32(150,103,239,255),5,.5f);
    dl->AddRectFilled(p,ImVec2(p.x+size.x,p.y+size.y),IM_COL32(10,13,21,238),5.f);
    dl->AddRect(p,ImVec2(p.x+size.x,p.y+size.y),IM_COL32(55,58,65,210),5.f);
}
static void Bar(ImDrawList* dl,ImVec2 p,float w,float h,float fraction,ImU32 color,const char* number,float size) {
    dl->AddRectFilled(p,ImVec2(p.x+w,p.y+h),IM_COL32(17,19,22,245),2.f);
    dl->AddRectFilled(p,ImVec2(p.x+w*Clamp(fraction,0,1),p.y+h),color,2.f);
    if(number&&*number)Text(dl,ImVec2(p.x+w*.5f,p.y+(h-size)*.5f),number,IM_COL32_WHITE,size,true);
}
static void Icon(ImDrawList* dl,const char* category,const char* name,ImVec2 p,float size,int level,float cd,bool known,int charges=-1,int maxLevel=0) {
    ID3D11ShaderResourceView* image=Texture(category,name);
    dl->AddRectFilled(p,ImVec2(p.x+size,p.y+size),IM_COL32(27,30,35,245));
    if(image)dl->AddImage(ImTextureRef((void*)image),p,ImVec2(p.x+size,p.y+size));
    else {
        const char* label=name&&*name?name:"?";
        char fallbackLabel[5] = {};
        strncpy(fallbackLabel, label, sizeof(fallbackLabel) - 1);
        Text(dl, ImVec2(p.x + 3, p.y + size * .4f), fallbackLabel, IM_COL32(210,214,224,255), 10.f);
    }
    if((known&&cd>.05f)||level==0)dl->AddRectFilled(p,ImVec2(p.x+size,p.y+size),IM_COL32(0,0,0,155));
    dl->AddRect(p,ImVec2(p.x+size,p.y+size),known&&cd<=.05f&&level!=0?IM_COL32(110,218,103,230):IM_COL32(120,125,138,230),0,0,1.f);
    char value[24];
    if(level>=0 && maxLevel>0) {
        const float gap=2.f,h=4.f,segment=(size-gap*(maxLevel-1))/maxLevel;
        for(int i=0;i<maxLevel;++i)dl->AddRectFilled(ImVec2(p.x+i*(segment+gap),p.y+size+3),ImVec2(p.x+i*(segment+gap)+segment,p.y+size+3+h),
            i<level?IM_COL32(78,221,107,255):IM_COL32(47,63,52,220),1.f);
    }
    if(!known && level!=0)Text(dl,ImVec2(p.x+size*.5f,p.y+(size-std::min(17.f,size*.5f))*.5f),"?",IM_COL32_WHITE,std::min(17.f,size*.5f),true,true);
    else if(cd>.05f){snprintf(value,sizeof(value),"%d",(int)ceilf(cd));Text(dl,ImVec2(p.x+size*.5f,p.y+(size-std::min(18.f,size*.52f))*.5f),value,IM_COL32_WHITE,std::min(18.f,size*.52f),true,true);}
    if(charges>=0){snprintf(value,sizeof(value),"%d",charges);float textSize=std::min(13.f,size*.35f);float y=(known&&cd>.05f)?p.y+1:p.y+size-textSize-1;
        Text(dl,ImVec2(p.x+size*.5f,y),value,IM_COL32_WHITE,textSize,true);}
}
static const ItemInfo* Special(const FrameUnit& u,const char* name) {
    for(int i=0;i<u.itemN;++i)if(!strcmp(u.items[i].icon,name) && !(u.items[i].slot>=9 && u.items[i].slot<=14))return &u.items[i];
    return nullptr;
}
void UpdateAegis(Frame& f) {
    if(!f.ok||!std::isfinite(f.now))return;
    g_aegisTracker.BeginFrame(f.now);
    for(auto& u:f.units){
        if(u.kind!=UnitKind::Hero||u.illusion)continue;
        const ItemInfo* item=Special(u,"aegis");
        auto value=g_aegisTracker.Observe(u.addr,item?item->addr:0,item?item->instanceHandle:0,
                                        item!=nullptr,u.inventoryRead,item?item->expiresAt:-1.f,f.now,u.alive);
        u.aegisVisible=value.visible;u.aegisEstimated=value.estimated;u.aegisSeconds=value.seconds;
    }
}
static void AegisTime(const FrameUnit& u,char* text,size_t cap) {
    char value[20];Time(u.aegisSeconds,value,sizeof(value));
    snprintf(text,cap,"%s%s",u.aegisEstimated?"~":"",value);
}
static void Aegis(ImDrawList* dl,const Frame& f,const FrameUnit& u,ImVec2 p,float size) {
    (void)f;if(!u.aegisVisible)return;
    Icon(dl,"items","aegis",p,size,-1,0,true);
    char timer[24];AegisTime(u,timer,sizeof(timer));
    Panel(dl,ImVec2(p.x+size+2,p.y),ImVec2(64,size));
    Text(dl,ImVec2(p.x+size+34,p.y+(size-16.f)*.5f),timer,
         u.aegisEstimated?IM_COL32(247,186,96,255):IM_COL32_WHITE,16.f,true,true);
}

bool WorldHeroVisible(const Frame& f,const FrameUnit& u){return localvisibility::Get(f,u).visible;}
void BeginWorldFrame(const Frame& f) {
    g_fogMarkers.clear();
    if(!f.ok||f.observedOnly||!cfg::espHeroes||cfg::vbe||f.localTeam<2||f.localTeam>3||!std::isfinite(f.now)){
        g_fogMemory.Reset();return;
    }
    if(g_fogTeam!=f.localTeam||g_fogRules!=f.rules||g_fogOwner!=f.localHandle){g_fogMemory.Reset();g_fogTeam=f.localTeam;g_fogRules=f.rules;g_fogOwner=f.localHandle;}
    g_fogMemory.Begin(f.now);
    for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.team!=f.localTeam&&(u.team==2||u.team==3)){
        auto vision=localvisibility::Get(f,u);
        g_fogMemory.Observe(u.entityHandle,vision.visible?u.alive:true,vision.known,vision.visible,{u.pos.x,u.pos.y,u.pos.z},StartsWith(u.name,"npc_dota_hero_")?u.name:u.nick);
    }
    g_fogMarkers=g_fogMemory.End();
}
void DrawFogMarkers() {
    if(cfg::menuOpen)return;
    auto* dl=ImGui::GetBackgroundDrawList();
    for(const auto& marker:g_fogMarkers){ImVec2 c;Vec3 p{marker.pos.x,marker.pos.y,marker.pos.z};
        if(!view::W2S(p,c)||!worldclip::Region(float(view::W),float(view::H),cfg::hudTop,cfg::hudTopY).Contains(c.x,c.y,23,43))continue;
        constexpr float r=20,pi=3.14159265f;float fraction=std::clamp(marker.remaining/10.f,0.f,1.f);
        dl->AddCircleFilled(c,r,IM_COL32(13,19,27,225),48);
        ImTextureID avatar=(ImTextureID)(uintptr_t)Texture("heroes",marker.portrait.c_str());
        if(avatar)heroinfo::RoundImage(dl,avatar,c,r-2.f);
        dl->AddCircle(c,r,IM_COL32(70,87,103,220),48,2);
        dl->PathArcTo(c,r,-pi*.5f,-pi*.5f+2*pi*fraction,48);
        dl->PathStroke(IM_COL32(237,181,91,255),0,2.5f);
        char value[12];snprintf(value,sizeof(value),"%d",(int)ceilf(marker.remaining));
        ImVec2 timer(c.x,c.y+r+12.f);
        dl->AddRectFilled(ImVec2(c.x-14,timer.y-9),ImVec2(c.x+14,timer.y+9),IM_COL32(13,19,27,225),4);
        heroinfo::NumericCentered(dl,theme::FontSmall(),14,timer,IM_COL32_WHITE,value);
    }
}
void DrawWorldHero(const Frame& f,const FrameUnit& u) {
    if(cfg::menuOpen||!u.alive||!WorldHeroVisible(f,u))return;
    ImVec2 foot,top;Vec3 raised{u.pos.x,u.pos.y,u.pos.z+u.hbOffset};
    if(!view::W2S(u.pos,foot)||!view::W2S(raised,top))return;
    int frame=ImGui::GetFrameCount();if(frame!=g_worldLayoutFrame){g_worldLayoutFrame=frame;g_worldLayout.occupied.clear();}
    heroinfo::Style style;style.hp=cfg::hudHpNumber;style.abilities=cfg::hudAbilities;style.items=cfg::hudItems;
    style.statuses=cfg::hudStatusBadges;style.illusions=cfg::hudIllusions;style.effects=cfg::showEffects;style.timedOnly=cfg::effectsTimedOnly;
    // Display-only colour group. Never infer local identity for automation from this.
    int displayTeam=f.ok?f.localTeam:0;
    if(!displayTeam&&mem::ValidPtr(game::g_sys.localCtrl)){uint8_t team=0;if(mem::Read(game::g_sys.localCtrl+off::BaseEntity::m_iTeamNum,team)&&(team==2||team==3))displayTeam=team;}
    bool red=displayTeam?u.team!=displayTeam:u.team==3;
    style.above=true;if(cfg::hudTop){auto box=compacttop::Place(float(view::W),float(view::H),2,0,cfg::hudTopY);if(box.valid)style.minOverlayY=box.y+box.h+12;}style.hpFont=theme::FontHp();style.pixels=cfg::hudSkillPixels;style.hpX=cfg::nativeHpOffsetX;style.hpY=heroinfo::HpOffset(cfg::nativeHpOffsetY,cfg::nativeHpRedOffsetY,red);
    auto overlay=Menu::GetOverlay();int relation=u.entityHandle==f.localHandle?4:(red?1:2);
    auto allowed=[&](const Menu::Layer& l){return l.enabled&&(l.showOn&relation)&&l.size>0;};
    style.layers=true;style.abilities=cfg::hudAbilities&&allowed(overlay.skills);style.items=cfg::hudItems&&allowed(overlay.items);style.effects=cfg::showEffects&&allowed(overlay.modifiers);
    style.hp=overlay.bars&&cfg::hudHpNumber;style.customBar=overlay.bars?overlay.bar:-1;
    style.skillAlign=overlay.skills.align;style.itemAlign=overlay.items.align;style.modAlign=overlay.modifiers.align;
    style.skillSize=overlay.skills.size;style.itemSize=overlay.items.size;style.modSize=overlay.modifiers.size;
    style.minified=overlay.skills.minified;style.accent=overlay.accent;
    auto mouse=ImGui::GetIO().MousePos;float dx=mouse.x-top.x,dy=mouse.y-top.y;bool hover=dx*dx+dy*dy<=overlay.hoverRadius*overlay.hoverRadius;
    auto alpha=[&](const Menu::Layer& l){return (hover?l.hoverOpacity:l.opacity)/100.f;};
    style.skillAlpha=alpha(overlay.skills);style.itemAlpha=alpha(overlay.items);style.modAlpha=alpha(overlay.modifiers);
    style.skillTheme=overlay.skills.themeColors;style.itemTheme=overlay.items.themeColors;style.modTheme=overlay.modifiers.themeColors;
    heroinfo::Draw(u,ImGui::GetBackgroundDrawList(),theme::FontSmall(),foot,top,view::W,view::H,ImGui::GetTime(),style,g_invisTracker,g_worldLayout,
        [](const char* category,const char* name)->ImTextureID{return (ImTextureID)(uintptr_t)Texture(category,name);});
}

static void Drag(float& x,float& y,float w,float h) {
    auto& io=ImGui::GetIO();
    static float* active=nullptr;
    if(!cfg::menuOpen){active=nullptr;return;}
    bool hover=io.MousePos.x>=x&&io.MousePos.x<=x+w&&io.MousePos.y>=y&&io.MousePos.y<=y+h;
    if(!active&&hover&&ImGui::IsMouseClicked(ImGuiMouseButton_Left))active=&x;
    if(!ImGui::IsMouseDown(ImGuiMouseButton_Left))active=nullptr;
    if(active==&x){x+=io.MouseDelta.x;y+=io.MouseDelta.y;}
    x=Clamp(x,0,std::max(0.f,io.DisplaySize.x-w));y=Clamp(y,0,std::max(0.f,io.DisplaySize.y-h));
}
static void Top(const Frame& f,ImDrawList* dl) {
    if(!cfg::hudTop||cfg::menuOpen||!f.ok||f.observedOnly)return;
    g_topRoster.Begin(f.now,f.rules,f.localHandle);
    for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&!u.illusion&&(u.team==2||u.team==3))g_topRoster.Observe(u.team,u.playerId,u.entityHandle,u.name);
    for(const auto& e:g_topRoster.Entries()){
        if(!e.handle)continue;auto box=compacttop::Place(ImGui::GetIO().DisplaySize.x,ImGui::GetIO().DisplaySize.y,e.team,e.slot,cfg::hudTopY);if(!box.valid)continue;
        const FrameUnit* unit=nullptr;for(const auto& u:f.units)if(u.entityHandle==e.handle&&u.team==e.team&&u.kind==UnitKind::Hero){unit=&u;break;}
        auto vision=unit?localvisibility::Get(f,*unit):localvisibility::State{};
        bool current=unit&&vision.known&&vision.visible;
        float x=box.x,y=box.y,w=box.w,size=box.icon;
        dl->PushClipRect({x,y},{x+w,y+box.h},true);
        Panel(dl,{x,y},{w,box.h});
        std::string portrait=fogmemory::PortraitKey(e.name.c_str());auto* avatar=Texture("heroes",portrait.c_str());
        if(cfg::hudPortraits&&avatar)dl->AddImage(ImTextureRef((void*)avatar),{x+4,y+4},{x+30,y+30});
        float barX=x+34,barW=w-38;char hp[24]="?",mana[24]="?";
        if(current){snprintf(hp,sizeof(hp),"%d",unit->hp);if(unit->manaRead&&unit->maxMana>0)snprintf(mana,sizeof(mana),"%d",int(unit->mana));else if(unit->manaRead&&unit->maxMana==0&&unit->mana==0)snprintf(mana,sizeof(mana),"--");}
        Bar(dl,{barX,y+4},barW,14,current&&unit->maxHp>0?float(unit->hp)/unit->maxHp:0,e.team==f.localTeam?IM_COL32(65,174,99,255):IM_COL32(215,71,83,255),hp,13);
        Bar(dl,{barX,y+20},barW,10,current&&unit->maxMana>0?unit->mana/unit->maxMana:0,IM_COL32(61,119,195,255),mana,10);
        if(!current||!unit->alive){const char* label=!current?(!unit?"--":vision.known?"FOG":"?"):"DEAD";Text(dl,{x+w*.5f,y+41},label,IM_COL32(181,190,205,255),12,true);dl->PopClipRect();continue;}
        // Skills only: no items, charges, level dots, Aegis/footer or empty second row.
        if(cfg::hudTopAbilities&&unit->abilN>0){
            int count=std::min(unit->abilN,6);float skillSize=std::min(size,(w-8-2*(count-1))/count);
            float rowX=x+(w-(skillSize*count+2*(count-1)))*.5f;
            for(int i=0;i<count;++i){const auto& a=unit->abil[i];Icon(dl,"abilities",a.icon,{rowX+i*(skillSize+2),y+36},skillSize,a.level,a.cd,a.cooldownRead,-1,0);}
        }
        dl->PopClipRect();
    }
}

static void Roshan(const Frame& f,ImDrawList* dl) {
    if(!cfg::hudRoshan)return;
    const float w=290,h=60;
    if(cfg::hudRoshanX<0)cfg::hudRoshanX=ImGui::GetIO().DisplaySize.x*.5f-w*.5f;
    Drag(cfg::hudRoshanX,cfg::hudRoshanY,w,h);ImVec2 p(cfg::hudRoshanX,cfg::hudRoshanY);Panel(dl,p,ImVec2(w,h));
    Text(dl,ImVec2(p.x+12,p.y+8),"ROSHAN / TIMING",IM_COL32_WHITE,15);
    char value[80];
    if(f.roshanAlive&&f.roshanMaxHp>0){snprintf(value,sizeof(value),"%d / %d",f.roshanHp,f.roshanMaxHp);Bar(dl,ImVec2(p.x+12,p.y+32),w-24,19,(float)f.roshanHp/f.roshanMaxHp,IM_COL32(223,78,65,255),value,13);}
    else if(f.roshanEtaLo>=0.f&&f.roshanEtaHi>=0.f){char a[24],b[24];Time(std::max(0.f,f.roshanEtaLo),a,sizeof(a));Time(std::max(0.f,f.roshanEtaHi),b,sizeof(b));snprintf(value,sizeof(value),"%s - %s",a,b);Text(dl,ImVec2(p.x+w*.5f,p.y+34),value,IM_COL32(238,175,95,255),16,true);}
    else Text(dl,ImVec2(p.x+12,p.y+34),"UNKNOWN / not observed",IM_COL32(155,159,170,255),13);
}
static void Watermark(ImDrawList* dl) {
    if(!cfg::hudWatermark)return;
    char line[160];snprintf(line,sizeof(line),"FDA  |  %.32s  |  %.0f UI FPS  |  ping --  |  MMR --",cfg::hudNickname,ImGui::GetIO().Framerate);
    float w=ImGui::GetFont()->CalcTextSizeA(14,10000,0,line).x+24,h=30;
    if(cfg::hudWatermarkX<0)cfg::hudWatermarkX=std::max(0.f,ImGui::GetIO().DisplaySize.x-w-12);
    Drag(cfg::hudWatermarkX,cfg::hudWatermarkY,w,h);ImVec2 p(cfg::hudWatermarkX,cfg::hudWatermarkY);Panel(dl,p,ImVec2(w,h));Text(dl,ImVec2(p.x+12,p.y+7),line,IM_COL32(230,232,239,255),14);
}
static void Bounty(const Frame& f,ImDrawList* dl) {
    if(!cfg::hudBounty||!f.queryUnit)return;
    for(const auto& u:f.units){if(u.addr!=f.queryUnit||u.kind!=UnitKind::Creep||!u.alive||u.bountyMin<0)continue;
        ImVec2 p;Vec3 pos{u.pos.x,u.pos.y,u.pos.z+u.hbOffset};if(!view::W2S(pos,p))return;
        // Query unit can remain selected; require cursor proximity to the unit as well.
        const auto mouse=ImGui::GetIO().MousePos;
        if(fabsf(mouse.x-p.x)>85.f||fabsf(mouse.y-p.y)>110.f)return;
        char value[40];if(u.bountyMin==u.bountyMax)snprintf(value,sizeof(value),"GOLD %d",u.bountyMin);else snprintf(value,sizeof(value),"GOLD %d-%d",u.bountyMin,u.bountyMax);
        Panel(dl,ImVec2(p.x+18,p.y-27),ImVec2(136,27));Text(dl,ImVec2(p.x+27,p.y-21),value,IM_COL32(245,196,86,255),15);return;
    }
}
void Draw(const Frame& f) {
    readProbe=ReadProbe{};
    for(const auto& unit:f.units) if(unit.kind==UnitKind::Hero) {
        ++readProbe.heroes;
        if(unit.buffsRead){++readProbe.buffLists;readProbe.buffEffects+=(int)unit.buffs.size();}else ++readProbe.buffUnknown;
        if(unit.inventoryRead)++readProbe.inventories;
        if(unit.aegisVisible&&unit.aegisEstimated)++readProbe.aegisEstimated;
        readProbe.inventorySlots+=std::max(0,unit.inventoryCount);readProbe.resolved+=unit.inventoryResolved;readProbe.unmapped+=unit.inventoryUnmapped;
        readProbe.items+=unit.itemN;readProbe.abilities+=unit.abilN;
        for(int i=0;i<unit.itemN;++i){if(unit.items[i].icon[0])++readProbe.mappedItems;if(!strcmp(unit.items[i].icon,"aegis")){++readProbe.aegis;if(unit.items[i].slot==-2)++readProbe.aegisFallback;if(unit.items[i].expiresAt>f.now&&unit.items[i].expiresAt-f.now<=1800.f)++readProbe.aegisTimed;}}
        for(int i=0;i<unit.abilN;++i)if(unit.abil[i].icon[0])++readProbe.mappedAbilities;
    }
    auto* dl=ImGui::GetBackgroundDrawList();Watermark(dl);
    if(!f.ok||cfg::menuOpen)return;
    Top(f,dl);Roshan(f,dl);Bounty(f,dl);
}
}
