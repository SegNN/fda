#include "hero_info.h"
#include <fstream>
#include <map>
#include <string>
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1120,640};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
 ImFont* font=io.Fonts->AddFontFromFileTTF("assets/fonts/ui-semibold.otf",15);if(!font)font=io.Fonts->AddFontDefault();
 unsigned char* px;int W,H;io.Fonts->GetTexDataAsRGBA32(&px,&W,&H);io.Fonts->SetTexID((ImTextureID)1);
 std::ofstream atlas("build/hud5-font-atlas.rgba",std::ios::binary);atlas.write((char*)px,W*H*4);atlas.close();
 ImGui::NewFrame();auto* dl=ImGui::GetBackgroundDrawList();heroinfo::InvisTracker tracker;heroinfo::Layout layout;heroinfo::Style style;
 std::map<std::string,int> textures;auto texture=[&](const char* category,const char* name)->ImTextureID{std::string key=std::string(category)+"/"+name;auto it=textures.find(key);if(it!=textures.end())return it->second;int id=2+(int)textures.size();textures[key]=id;return id;};
 heroinfo::Text(dl,font,22,{24,22},IM_COL32(241,245,249,255),"HUD5 / added overlay elements");
 heroinfo::Text(dl,font,13,{24,55},IM_COL32(155,173,190,255),"Synthetic data. Native Dota HP bars and hero levels are NOT drawn here; only the added text and indicators.");
 const char* titles[]={"Huskar / items + Flask","Phantom Lancer / illusion + stun","Templar Assassin / INVIS"};
 for(int n=0;n<3;++n){float x=190+370*n;
  dl->AddRectFilled({x-162,96},{x+162,600},IM_COL32(15,22,29,205),8);dl->AddRect({x-162,96},{x+162,600},IM_COL32(52,66,77,230),8);
  heroinfo::Text(dl,font,15,{x,112},IM_COL32(213,222,232,255),titles[n],true);

  FrameUnit u;u.kind=UnitKind::Hero;u.entityHandle=0x8001+n;u.alive=true;u.hp=n==0?582:n==1?410:760;u.maxHp=1000;u.level=7+n;
  u.clockRead=true;u.sampleTime=101;u.illusionRead=true;u.stateRead=true;u.inventoryRead=true;u.buffsRead=true;u.abilN=3;
  const char* skills[]={"huskar_inner_fire","huskar_burning_spear","huskar_berserkers_blood"};
  for(int i=0;i<3;++i){strcpy(u.abil[i].icon,skills[i]);u.abil[i].level=i==0?2:1;u.abil[i].maxLevel=4;u.abil[i].cooldownRead=true;u.abil[i].cd=i==0?4.f:0;}
  u.itemN=3;const char* items[]={"armlet","power_treads","flask"};for(int i=0;i<3;++i){u.items[i].slot=i;strcpy(u.items[i].icon,items[i]);u.items[i].cooldownRead=true;u.items[i].cd=i==2?1.f:0;u.items[i].charges=i==2?1:-1;}
  if(n==0){buffreader::Buff b;strcpy(b.name,"modifier_flask_healing");b.duration=10;b.created=100;b.expires=110;u.buffs={b};}
  if(n==1){u.illusion=true;u.unitState=uint64_t(1)<<5;}
  if(n==2){u.unitState=uint64_t(1)<<7;tracker.Observe(u.entityHandle,true,true,true,0);for(int i=1;i<35;++i)tracker.Observe(u.entityHandle,true,true,true,i*.1);u.itemN=1;}
  heroinfo::Draw(u,dl,font,{x,320},{x,215},1120,640,3.5,style,tracker,layout,texture);
 }
 ImGui::Render();std::ofstream out("build/hud5-draw-data.json");out<<"{\"atlas\":["<<W<<","<<H<<"],\"textures\":{";bool first=true;for(auto& t:textures){if(!first)out<<",";first=false;out<<"\""<<t.second<<"\":\""<<t.first<<"\"";}out<<"},\"vertices\":[";
 for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}
 out<<"],\"indices\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"commands\":[";
 for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}
 out<<"]}";out.close();ImGui::DestroyContext();
}
