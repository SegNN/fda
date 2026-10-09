#include "hero_info.h"
#include "fog_memory.h"
#include "local_visibility.h"
#include "compact_top.h"
#include "world_clip.h"
#include "combo_profiles.h"
#include <cassert>
#include <fstream>
#include <map>
#include <iostream>
struct ID3D11ShaderResourceView;
namespace theme {ImFont* FontSmall(){return ImGui::GetFont();}ImFont* FontBig(){return ImGui::GetFont();}ImFont* FontHp(){return ImGui::GetFont();}}
namespace view {int W=1920,H=1080;bool W2S(const Vec3& p,ImVec2& c){c={p.x,p.y};return true;}}
static std::map<std::string,int> textures;
namespace hud {
static int g_fogTeam=0;static uintptr_t g_fogRules=0;static uint32_t g_fogOwner=0;static fogmemory::Tracker g_fogMemory;static std::vector<fogmemory::Marker> g_fogMarkers;static compacttop::Roster g_topRoster;
static ID3D11ShaderResourceView* Texture(const char* category,const char* name){std::string path=std::string("assets/")+category+"/"+name+".png";if(!std::ifstream(path).good())return nullptr;auto it=textures.find(path);if(it==textures.end())it=textures.emplace(path,int(textures.size()+2)).first;return reinterpret_cast<ID3D11ShaderResourceView*>(uintptr_t(it->second));}
#include "production_top_primitives.inc"
#include "production_aegis_time.inc"
#include "production_fog.inc"
#include "production_top.inc"
}
static FrameUnit Unit(int n){FrameUnit u;u.entityHandle=0x8001+n;u.kind=UnitKind::Hero;u.team=n<5?2:3;u.playerId=n;u.hp=5659-100*n;u.maxHp=6000;u.mana=500;u.maxMana=900;u.manaRead=true;u.alive=u.clockRead=u.stateRead=u.inventoryRead=u.buffsRead=true;u.sampleTime=100;u.npcVisibilityRead=u.npcVisible=true;u.teamVisibilityRead=true;u.teamVisibilityMask=0;
 if(n==0){strcpy(u.name,"npc_dota_hero_huskar");const char* a[]={"huskar_inner_fire","huskar_burning_spear","huskar_berserkers_blood","huskar_life_break"};u.abilN=4;for(int j=0;j<4;++j){strcpy(u.abil[j].icon,a[j]);u.abil[j].level=u.abil[j].maxLevel=j==3?3:4;u.abil[j].cooldownRead=true;u.abil[j].cd=j==0?30:0;}}
 else {const auto& p=combos::profiles[n-1];strcpy(u.name,p.hero);u.abilN=p.count;for(int j=0;j<p.count;++j){strcpy(u.abil[j].icon,p.steps[j].ability);u.abil[j].level=u.abil[j].maxLevel=j==p.count-1&&p.steps[j].defaultKey=='R'?3:4;u.abil[j].cooldownRead=true;u.abil[j].cd=j==0?9:0;}}
 const char* items[]={"armlet","black_king_bar","overwhelming_blink","heart","tango","ultimate_scepter"};u.itemN=6;for(int j=0;j<6;++j){auto& item=u.items[j];item.slot=j;strcpy(item.icon,items[j]);item.cooldownRead=true;item.cd=j==1?42:0;item.charges=j==4?3:-1;}return u;}
int main(int argc,char** argv){cfg::menuOpen=false;int width=argc>1?std::stoi(argv[1]):1920;bool hidden=argc>2;int height=width==1920?1080:720;view::W=width;view::H=height;ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={float(width),float(height)};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
 auto* font=io.Fonts->AddFontFromFileTTF("assets/fonts/ui-semibold.otf",15);if(!font)font=io.Fonts->AddFontDefault();unsigned char* px;int fw,fh;io.Fonts->GetTexDataAsRGBA32(&px,&fw,&fh);io.Fonts->SetTexID((ImTextureID)1);
 std::string stem="build/esp19-"+std::to_string(width)+(hidden?"-hidden":"");std::ofstream atlas(stem+"-font.rgba",std::ios::binary);atlas.write((char*)px,fw*fh*4);atlas.close();
 ImGui::NewFrame();auto* dl=ImGui::GetBackgroundDrawList();dl->AddRectFilled({0,0},{float(width),float(height)},IM_COL32(12,18,25,255));
 Frame f;f.ok=f.localAlive=true;f.localHandle=0x8001;f.localTeam=2;f.rules=1;f.now=100;for(int i=0;i<10;++i)f.units.push_back(Unit(i));if(hidden){hud::g_topRoster.Begin(f.now,f.rules,f.localHandle);for(const auto& u:f.units)hud::g_topRoster.Observe(u.team,u.playerId,u.entityHandle,u.name);for(int i=5;i<8;++i)f.units[i].npcVisible=false;f.units[8].npcVisibilityRead=false;f.units.pop_back();}hud::Top(f,dl);
 heroinfo::Text(dl,font,20,{24,208},IM_COL32_WHITE,"ESP19 / actual C++ HUD render test");heroinfo::Text(dl,font,14,{24,239},IM_COL32(159,178,197,255),(hidden?"Synthetic data: fog / unknown / unavailable NPCs show no current stats. The world ring uses a frozen last-seen position.":"Synthetic data, not a live Dota capture. Armlet + 9 combo profiles retained. Enemy model masks are all zero; team NPC table says visible."));
 if(width==1920){heroinfo::InvisTracker tracker;heroinfo::Layout layout;heroinfo::Style style;style.above=true;style.pixels=28;
  auto texture=[](const char* cat,const char* name)->ImTextureID{return (ImTextureID)(uintptr_t)hud::Texture(cat,name);};
  const char* labels[]={"Ally / horizontal rows ABOVE bar",hidden?"Enemy hidden / no current world stats":"Enemy / zero model mask + visible NPC bit"};
  for(int n=0;n<2;++n){auto& u=f.units[n==0?0:5];float x=n==0?420:1080;heroinfo::Text(dl,font,15,{x,665},IM_COL32_WHITE,labels[n],true);if(localvisibility::Get(f,u).visible)heroinfo::Draw(u,dl,font,{x,625},{x,545},width,height,100,style,tracker,layout,texture);if(!(hidden&&n==1)){dl->AddLine({x-90,532},{x+90,532},IM_COL32(96,112,127,255),2);heroinfo::Text(dl,font,13,{x,556},IM_COL32(152,172,190,255),"native HP-bar anchor (guide only)",true);}}
  Frame fog;fog.ok=true;fog.localTeam=2;fog.localHandle=0x8001;fog.rules=2;fog.now=100;auto enemy=Unit(5);enemy.pos={1540,530,0};fog.units={enemy};hud::BeginWorldFrame(fog);assert(hud::g_fogMarkers.empty()&&hud::WorldHeroVisible(fog,enemy));
  fog.now=101;fog.units[0].npcVisible=false;fog.units[0].pos={1900,980,0};hud::BeginWorldFrame(fog);assert(hud::g_fogMarkers.size()==1&&hud::g_fogMarkers[0].pos.x==1540);
  fog.now=106;hud::BeginWorldFrame(fog);assert(hud::g_fogMarkers.size()==1&&hud::g_fogMarkers[0].remaining==5);hud::DrawFogMarkers();heroinfo::Text(dl,font,15,{1540,665},IM_COL32_WHITE,"Fog marker / frozen last-seen position / 5s",true);
  fog.now=107;fog.units[0].npcVisible=true;hud::BeginWorldFrame(fog);assert(hud::g_fogMarkers.empty());
 }
 ImGui::Render();std::ofstream out(stem+"-mesh.json");out<<"{\"screen\":["<<width<<","<<height<<"],\"font\":["<<fw<<","<<fh<<"],\"textures\":{";bool comma=false;for(auto& t:textures){if(comma)out<<",";comma=true;out<<"\""<<t.second<<"\":\""<<t.first<<"\"";}out<<"},\"lists\":[{\"v\":[";
 for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}
 out<<"],\"idx\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"cmd\":[";for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}out<<"]}]}";out.close();ImGui::DestroyContext();std::cout<<"PASS production compact Top + above-bar World rows + visible->hidden frozen marker at "<<width<<" px; synthetic memory only.\n";
}
