#include "hud.h"
#include "hook.h"
#include "top_anchor.h"
#include "theme.h"
#include "aegis_tracker.h"
#include "imgui.h"
#include <wincodec.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <cmath>

namespace hud {
ReadProbe readProbe;
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
    dl->AddRectFilled(p,ImVec2(p.x+size.x,p.y+size.y),IM_COL32(13,15,18,225),5.f);
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

void DrawWorldHero(const Frame& f,const FrameUnit& u) {
    if(!u.alive)return;
    ImVec2 foot,top;
    if(!view::W2S(u.pos,foot))return;
    Vec3 raised{u.pos.x,u.pos.y,u.pos.z+u.hbOffset};
    if(!view::W2S(raised,top))return;
    auto* dl=ImGui::GetBackgroundDrawList();
    const float scale=Clamp(cfg::hudIconScale,.6f,1.6f),width=150.f*scale;
    const float icon=Clamp(cfg::hudSkillPixels,28.f,60.f);
    ImVec2 p(top.x-width*.5f,top.y-38.f*scale);
    // Native health bar is left untouched by default. This optional panel is additive.
    if(cfg::hudWorld) {
        Panel(dl,ImVec2(p.x-34.f*scale,p.y-3),ImVec2(width+72.f*scale,43.f*scale));
        if(cfg::hudPortraits){auto* portrait=Texture("heroes",StartsWith(u.name,"npc_dota_hero_")?u.name+strlen("npc_dota_hero_"):u.nick);if(portrait)dl->AddImage(ImTextureRef((void*)portrait),ImVec2(p.x-32.f*scale,p.y),ImVec2(p.x-3,p.y+28.f*scale));}
        char value[64];snprintf(value,sizeof(value),"%d",u.hp);
        ImU32 hp=u.illusion&&cfg::hudIllusions?IM_COL32(178,116,240,255):(u.team!=f.localTeam?IM_COL32(237,62,71,255):IM_COL32(69,197,90,255));
        Bar(dl,p,width,21.f*scale,u.maxHp>0?(float)u.hp/u.maxHp:0,hp,cfg::hudHpNumber?value:"",18.f*scale);
        snprintf(value,sizeof(value),"%d",(int)u.mana);
        Bar(dl,ImVec2(p.x,p.y+24.f*scale),width,13.f*scale,u.maxMana>0?u.mana/u.maxMana:0,IM_COL32(54,183,225,255),cfg::hudManaNumber?value:"",11.f*scale);
        snprintf(value,sizeof(value),"%d",u.level);Text(dl,ImVec2(p.x+width+5,p.y+4),value,IM_COL32_WHITE,18.f*scale);
    }
    if(cfg::hudAbilities){
        const int count=u.abilN;const int cols=std::min(count,6);
        for(int i=0;i<count;++i){int row=i/6,column=i%6,inRow=std::min(6,count-row*6);float span=inRow*(icon+3)-3;
            const auto& a=u.abil[i];Icon(dl,"abilities",a.icon,ImVec2(foot.x-span*.5f+column*(icon+3),foot.y+12+row*(icon+12)),icon,a.level,a.cd,a.cooldownRead,-1,a.maxLevel);}
        (void)cols;
    }
    if(cfg::hudVisibleByEnemy && u.team==f.localTeam) {
        const int enemy=f.localTeam==2?3:2;
        const char* flag=!u.teamVisibilityRead||cfg::vbe?"VISION ?":((u.teamVisibilityMask&(1u<<enemy))?"SEEN BY ENEMY*":"MASK NOT SET*");
        Text(dl,ImVec2(top.x,top.y-60),flag,IM_COL32(247,186,96,255),14,true);
    }
    if(cfg::hudStatusBadges) {
        // Only effects already identified by the verified reader; no invented buff icons.
        int row=0;const float badgeX=foot.x+std::min(u.abilN,6)*(icon+3)*.5f+10;
        if(u.illusion){Panel(dl,ImVec2(badgeX,foot.y+12),ImVec2(72,24));Text(dl,ImVec2(badgeX+7,foot.y+17),"ILLUSION",IM_COL32(200,165,255,255),12);++row;}
        if(u.invis>.4f){Panel(dl,ImVec2(badgeX,foot.y+12+row*27),ImVec2(72,24));Text(dl,ImVec2(badgeX+7,foot.y+17+row*27),"INVIS",IM_COL32(200,165,255,255),12);}
    }
    if(cfg::hudItems){int row=0;const float itemSize=icon;
        for(int i=0;i<u.itemN&&row<8;++i){const auto& it=u.items[i];if(it.slot>5&&it.slot!=16)continue;if(!it.icon[0])continue;if(!strcmp(it.icon,"aegis")||!strcmp(it.icon,"tpscroll"))continue;
            Icon(dl,"items",it.icon,ImVec2(top.x+width*.5f+10,top.y+row*(itemSize+3)),itemSize,-1,it.cd,it.cooldownRead,it.charges>0?it.charges:-1);++row;}
        Aegis(dl,f,u,ImVec2(top.x+width*.5f+10,top.y-itemSize-4),itemSize);
        if(const auto* tp=Special(u,"tpscroll"))Icon(dl,"items","tpscroll",ImVec2(top.x+width*.5f+10,top.y+row*(itemSize+3)),itemSize,-1,tp->cd,tp->cooldownRead,tp->charges);
    }
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
    if(!cfg::hudTop)return;
    const float size=Clamp(cfg::hudSkillPixels,28.f,60.f),gap=3.f;
    for(const auto& u:f.units){
        if(u.kind!=UnitKind::Hero||u.illusion)continue;
        topanchor::Anchor anchor;if(!topanchor::Find(u,anchor))continue;
        // 35 px requested even when native portraits are narrower. Icons use one
        // column until the confirmed portrait is wide enough for two; never overlap neighbours.
        const int cols=std::max(1,(int)((anchor.w-4+gap)/(size+gap)));
        const float w=std::max(anchor.w,size+4),x=anchor.x+(anchor.w-w)*.5f,y=anchor.y+anchor.h+3;
        const int rows=cfg::hudTopAbilities?(u.abilN+cols-1)/cols:0;
        const float height=38.f+(rows?rows*(size+10):20.f);
        Panel(dl,ImVec2(x,y),ImVec2(w,height));char value[32];
        snprintf(value,sizeof(value),"%d",u.hp);Bar(dl,ImVec2(x+1,y+1),w-2,17,u.maxHp>0?(float)u.hp/u.maxHp:0,u.team==f.localTeam?IM_COL32(67,188,76,255):IM_COL32(229,56,69,255),cfg::hudHpNumber?value:"",14);
        snprintf(value,sizeof(value),"%d",(int)u.mana);Bar(dl,ImVec2(x+1,y+20),w-2,14,u.maxMana>0?u.mana/u.maxMana:0,IM_COL32(56,112,220,255),cfg::hudManaNumber?value:"",12);
        if(cfg::hudTopAbilities)for(int i=0;i<u.abilN;++i){
            const auto& a=u.abil[i];const float span=cols*(size+gap)-gap;
            Icon(dl,"abilities",a.icon,ImVec2(x+(w-span)*.5f+(i%cols)*(size+gap),y+38+(i/cols)*(size+10)),size,a.level,a.cd,a.cooldownRead,-1,a.maxLevel);
        }
        if(!rows){snprintf(value,sizeof(value),"L%d",u.level);Text(dl,ImVec2(x+5,y+39),value,IM_COL32_WHITE,13);}
        // Aegis has a dedicated row independent of ability mode. Timer sits
        // below its icon in narrow top cards, avoiding overlap with adjacent heroes.
        if(u.aegisVisible){
            ImVec2 p(x+(w-size)*.5f,y+height+4);Icon(dl,"items","aegis",p,size,-1,0,true);
            char timer[24];AegisTime(u,timer,sizeof(timer));
            Panel(dl,ImVec2(x,p.y+size+2),ImVec2(w,24));
            Text(dl,ImVec2(x+w*.5f,p.y+size+6),timer,
                 u.aegisEstimated?IM_COL32(247,186,96,255):IM_COL32_WHITE,14,true,true);
        }
        if(cfg::hudVisibleByEnemy&&u.team==f.localTeam){
            const int enemy=f.localTeam==2?3:2;
            const char* flag=!u.teamVisibilityRead||cfg::vbe?"VISION ?":((u.teamVisibilityMask&(1u<<enemy))?"SEEN*":"MASK 0*");
            Text(dl,ImVec2(x+w*.5f,y-18),flag,IM_COL32(247,186,96,255),12,true);
        }
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
    if(!f.ok)return;
    Top(f,dl);Roshan(f,dl);Bounty(f,dl);
}
}
