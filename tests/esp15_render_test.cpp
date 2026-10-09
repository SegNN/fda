#include "hero_info.h"
#include "fog_memory.h"
#include "local_visibility.h"
#include <fstream>
#include <cassert>
#include <iostream>
namespace theme {ImFont* FontSmall(){return ImGui::GetFont();}}
namespace view {int W=1120,H=640;bool W2S(const Vec3& p,ImVec2& c){c={p.x,p.y};return true;}}
namespace hud {
static void* Texture(const char* category,const char* name){return !strcmp(category,"heroes")&&!strcmp(name,"huskar")?(void*)2:nullptr;}
static int g_fogTeam=0;static uintptr_t g_fogRules=0;static uint32_t g_fogOwner=0;static fogmemory::Tracker g_fogMemory;static std::vector<fogmemory::Marker> g_fogMarkers;
#include "production_fog.inc"
}
int main(){ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1120,640};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
 auto* font=io.Fonts->AddFontFromFileTTF("assets/fonts/ui-semibold.otf",15);if(!font)font=io.Fonts->AddFontDefault();unsigned char* px;int W,H;io.Fonts->GetTexDataAsRGBA32(&px,&W,&H);io.Fonts->SetTexID((ImTextureID)1);
 std::ofstream atlas("build/esp15-font.rgba",std::ios::binary);atlas.write((char*)px,W*H*4);atlas.close();ImGui::NewFrame();auto* dl=ImGui::GetBackgroundDrawList();
 heroinfo::Text(dl,font,22,{28,24},IM_COL32_WHITE,"ESP16 / portrait + observed-vision timer");
 heroinfo::Text(dl,font,14,{28,60},IM_COL32(166,180,197,255),"Synthetic data. Rings below use the production HUD drawing function; no live Dota capture.");
 Frame f;f.ok=true;f.localTeam=2;FrameUnit u;strcpy(u.name,"npc_dota_hero_huskar");u.kind=UnitKind::Hero;u.team=3;u.entityHandle=0x8001;u.alive=true;u.npcVisibilityRead=true;u.npcVisible=true;u.teamVisibilityRead=true;u.teamVisibilityMask=1u<<2;
 const char* labels[]={"Visible","Missing: 10 seconds","Missing: 5 seconds","Expired: no marker","Visible again: no marker"};
 for(int n=0;n<5;++n){float x=122+219*n;dl->AddRectFilled({x-98,125},{x+98,535},IM_COL32(16,23,31,255),8);dl->AddRect({x-98,125},{x+98,535},IM_COL32(54,68,84,255),8);
  heroinfo::Text(dl,font,13,{x,150},IM_COL32_WHITE,labels[n],true);
  hud::g_fogMemory.Reset();f.now=100;u.pos={x,320,0};u.npcVisible=true;u.teamVisibilityMask=0;f.units={u};hud::BeginWorldFrame(f);assert(hud::g_fogMarkers.empty()&&hud::WorldHeroVisible(f,u));
  if(n){f.now=101;f.units[0].npcVisible=false;f.units[0].teamVisibilityMask=0;hud::BeginWorldFrame(f);assert(hud::g_fogMarkers.size()==1);
    if(n>=2){f.now=n==3?111:106;hud::BeginWorldFrame(f);}if(n==4){f.units[0].npcVisible=true;f.units[0].teamVisibilityMask=0;hud::BeginWorldFrame(f);}
  }
  if(n==1)assert(hud::g_fogMarkers[0].remaining==10&&hud::g_fogMarkers[0].portrait=="huskar");if(n==2)assert(hud::g_fogMarkers[0].remaining==5);if(n==3||n==4)assert(hud::g_fogMarkers.empty());
  int before=dl->VtxBuffer.Size;hud::DrawFogMarkers();if(n==1||n==2)assert(dl->VtxBuffer.Size>before);else assert(dl->VtxBuffer.Size==before);
  if(n==0||n==4){ // UI-only representative hero anchor; not part of production marker renderer.
    dl->AddCircleFilled({x,320},17,IM_COL32(47,113,137,255),32);heroinfo::Text(dl,font,13,{x,361},IM_COL32(166,180,197,255),"Normal HeroInfo",true);
  }
  heroinfo::Text(dl,font,13,{x,470},IM_COL32(166,180,197,255),(n==1||n==2)?"No HP / skills / items":"No stale fog UI",true);
 }
 u.npcVisibilityRead=false;u.teamVisibilityRead=false;assert(!hud::WorldHeroVisible(f,u));u.teamVisibilityRead=true;cfg::vbe=true;assert(!hud::WorldHeroVisible(f,u));hud::BeginWorldFrame(f);assert(hud::g_fogMarkers.empty());cfg::vbe=false;f.observedOnly=true;assert(!hud::WorldHeroVisible(f,u));hud::BeginWorldFrame(f);assert(hud::g_fogMarkers.empty());
 ImGui::Render();std::ofstream out("build/esp15-mesh.json");out<<"{\"font\":["<<W<<","<<H<<"],\"textures\":{\"2\":\"assets/heroes/huskar.png\"},\"lists\":[{\"v\":[";
 for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}
 out<<"],\"idx\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"cmd\":[";
 for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}out<<"]}]}";out.close();ImGui::DestroyContext();
 std::cout<<"PASS production visibility gates / BeginWorldFrame / DrawFogMarkers: 10s,5s,expiry,reappearance,unknown,forced-vision and observed-only guards. Synthetic geometry only.\n";
}
