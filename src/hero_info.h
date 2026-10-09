#pragma once
#include "game.h"
#include "imgui.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace heroinfo {
struct Rect {float x=0,y=0,w=0,h=0;};
struct Layout {
 std::vector<Rect> occupied;
 bool Place(float x,float y,float w,float h,int W,int H,Rect& out){
  if(w>W-16||h>H-16||W<32||H<32)return false;
  // Fixed hero-local anchor. Never move another hero's row on collision.
  // At the viewport boundary clip the row instead of teleporting it above the hero.
  if(x+w<=0||x>=W||y+h<=0||y>=H)return false;
  out={x,y,w,h};occupied.push_back(out);return true;
 }
};
struct Invisible {bool known=false,active=false;double started=0,lastSeen=0;float remaining=0;};
class InvisTracker {
 std::unordered_map<uint32_t,Invisible> states;
public:
 Invisible Observe(uint32_t h,bool readable,bool invis,bool alive,double now){
  if(!h||!std::isfinite(now))return {};
  for(auto it=states.begin();it!=states.end();)if(now<it->second.lastSeen||now-it->second.lastSeen>2.)it=states.erase(it);else ++it;
  auto& s=states[h];if(!alive){s=Invisible{};s.lastSeen=now;return s;}
  if(!readable){s.lastSeen=now;return {};}
  if(invis&&(!s.known||!s.active))s.started=now;
  s.known=true;s.active=invis;s.lastSeen=now;
  s.remaining=invis?std::clamp(float(7.-(now-s.started)),0.f,7.f):0.f;
  return s;
 }
 void Reset(){states.clear();}
};
inline float HpOffset(float common,float redOffset,bool red){return common+(red?redOffset:0.f);}
struct Style {bool hp=true,abilities=true,items=true,statuses=true,illusions=true,effects=true,timedOnly=true,above=false;float pixels=30,hpX=0,hpY=0,minOverlayY=0;ImFont* hpFont=nullptr;};
struct Stats {int items=0,abilities=0,effects=0,statuses=0;bool invis=false,illusion=false;float invisRemaining=0;};
inline int Charges(const ItemInfo& item){return item.charges>0?item.charges:-1;}
inline bool Bit(uint64_t v,int b){return (v&(uint64_t(1)<<b))!=0;}
inline bool Illusion(const FrameUnit& u){
 if(u.illusionRead&&u.illusion)return true;
 if(u.buffsRead||u.buffsVisualRead)for(auto& b:u.buffs)if(!strcmp(b.name,"modifier_illusion")||!strcmp(b.name,"modifier_phantom_lancer_juxtapose_illusion")||!strcmp(b.name,"modifier_phantom_lancer_doppelwalk_illusion"))return true;
 return false;
}
inline void Text(ImDrawList* dl,ImFont* font,float size,ImVec2 pos,ImU32 color,const char* s,bool center=false){
 if(!font)font=ImGui::GetFont();if(center)pos.x-=font->CalcTextSizeA(size,10000,0,s).x*.5f;
 for(auto d:{ImVec2(-1,0),ImVec2(1,0),ImVec2(0,-1),ImVec2(0,1)})dl->AddText(font,size,ImVec2(pos.x+d.x,pos.y+d.y),IM_COL32(0,0,0,240),s);
 dl->AddText(font,size,pos,color,s);
}
inline Rect NumericInk(ImFont* font,float size,const char* text){
 if(!font)font=ImGui::GetFont();auto* baked=font->GetFontBaked(size);
 float advance=0,left=1e9f,top=1e9f,right=-1e9f,bottom=-1e9f;
 for(const char* p=text;*p;++p){auto* g=baked->FindGlyph((ImWchar)(unsigned char)*p);if(!g)continue;
  if(g->Visible){left=std::min(left,advance+g->X0);right=std::max(right,advance+g->X1);top=std::min(top,g->Y0);bottom=std::max(bottom,g->Y1);}advance+=g->AdvanceX;}
 if(right<left)return {};return {left,top,right-left,bottom-top};
}
inline ImVec2 NumericOrigin(ImFont* font,float size,const char* text,ImVec2 center){
 auto box=NumericInk(font,size,text);return {std::round(center.x-box.x-box.w*.5f),std::round(center.y-box.y-box.h*.5f)};
}
inline void NumericCentered(ImDrawList* dl,ImFont* font,float size,ImVec2 center,ImU32 color,const char* text){
 Text(dl,font,size,NumericOrigin(font,size,text,center),color,text);
}
inline void Badge(ImDrawList* dl,ImFont* font,ImVec2 p,const char* label,ImU32 color,float width){
 dl->AddRectFilled(p,ImVec2(p.x+width,p.y+22),IM_COL32(16,20,26,235),4);
 dl->AddRect(p,ImVec2(p.x+width,p.y+22),color,4,0,1);
 Text(dl,font,13,ImVec2(p.x+width*.5f,p.y+3),color,label,true);
}
template<class Texture>
void Icon(ImDrawList* dl,ImFont* font,Texture texture,const char* cat,const char* name,ImVec2 p,float size,int level,int maxLevel,float cd,bool known,int charges){
 ImU32 edge=IM_COL32(110,135,157,240);dl->AddRectFilled(p,ImVec2(p.x+size,p.y+size),IM_COL32(18,24,31,235),3);
 ImTextureID image=texture(cat,name);if(image)dl->AddImage(ImTextureRef(image),p,ImVec2(p.x+size,p.y+size));
 else {char fallback[5]={};if(name)strncpy(fallback,name,4);Text(dl,font,12,ImVec2(p.x+size*.5f,p.y+size*.35f),IM_COL32(230,235,241,255),fallback[0]?fallback:"?",true);}
 if(level==0)dl->AddRectFilled(p,ImVec2(p.x+size,p.y+size),IM_COL32(0,0,0,175));
 if(level!=0&&known&&cd>.05f){dl->AddRectFilled(p,ImVec2(p.x+size,p.y+size),IM_COL32(0,0,0,160));char n[12];snprintf(n,sizeof(n),"%d",(int)ceilf(cd));Text(dl,font,16,ImVec2(p.x+size*.5f,p.y+size*.25f),IM_COL32_WHITE,n,true);}
 if(level!=0&&!known){Text(dl,font,14,ImVec2(p.x+size-5,p.y),IM_COL32_WHITE,"?",true);}
 dl->AddRect(p,ImVec2(p.x+size,p.y+size),edge,3,0,1);
 // Native-panel style: only max-level segments, never numeric ability levels.
 if(level>=0&&maxLevel>0&&maxLevel<=10){float gap=2,segment=(size-gap*(maxLevel-1))/maxLevel;
  for(int i=0;i<maxLevel;++i)dl->AddRectFilled(ImVec2(p.x+i*(segment+gap),p.y+size+4),ImVec2(p.x+i*(segment+gap)+segment,p.y+size+9),i<level?IM_COL32(224,208,140,255):IM_COL32(74,80,77,255));}
 if(charges>=0){char n[12];if(charges>99)strcpy(n,"99+");else snprintf(n,sizeof(n),"%d",charges);Text(dl,font,12,ImVec2(p.x+size*.5f,p.y+size+4),IM_COL32_WHITE,n,true);}
}
inline const char* EffectName(const char* raw,char* buf,int cap,const char*& icon){
 icon=nullptr;
 if(!strcmp(raw,"modifier_flask_healing")){icon="flask";return "Flask";}
 if(!strcmp(raw,"modifier_tango_heal")){icon="tango";return "Tango";}
 if(!strcmp(raw,"modifier_clarity_potion")){icon="clarity";return "Clarity";}
 const char* name=!strncmp(raw,"modifier_",9)?raw+9:raw;strncpy(buf,name,cap-1);buf[cap-1]=0;
 for(char* p=buf;*p;++p)if(*p=='_')*p=' ';return buf;
}
inline float EffectFraction(const buffreader::Buff& b,float remaining){
 if(!std::isfinite(remaining)||remaining<0||!std::isfinite(b.duration)||b.duration<=0)return -1;
 return std::clamp(remaining/b.duration,0.f,1.f);
}
// Texture fan clips pixels to a true circle, not a square under a round outline.
inline void RoundImage(ImDrawList* dl,ImTextureID image,ImVec2 center,float radius){
 const int n=48;dl->PushTexture(ImTextureRef(image));dl->PrimReserve(n*3,n+1);
 ImDrawIdx base=(ImDrawIdx)dl->_VtxCurrentIdx;
 dl->PrimWriteVtx(center,ImVec2(.5f,.5f),IM_COL32_WHITE);
 for(int i=0;i<n;++i){float angle=i*6.2831853f/n,c=cosf(angle),s=sinf(angle);
  dl->PrimWriteVtx(ImVec2(center.x+radius*c,center.y+radius*s),ImVec2(.5f+.5f*c,.5f+.5f*s),IM_COL32_WHITE);}
 for(int i=0;i<n;++i){dl->PrimWriteIdx(base);dl->PrimWriteIdx((ImDrawIdx)(base+1+i));dl->PrimWriteIdx((ImDrawIdx)(base+1+(i+1)%n));}
 dl->PopTexture();
}
template<class Texture>
ImTextureID EffectTexture(Texture texture,const buffreader::Buff& b){
 char clean[80]={};const char* item=nullptr;EffectName(b.name,clean,sizeof(clean),item);
 ImTextureID image=item?texture("items",item):0;
 // Try exact ability identifier, then known modifier suffixes. No fabricated texture.
 if(!image){char ability[128]={};const char* raw=!strncmp(b.name,"modifier_",9)?b.name+9:b.name;strncpy(ability,raw,127);
  image=texture("abilities",ability);
  const char* suffixes[]={"_debuff","_buff","_aura","_effect","_active","_slow","_damage","_heal"};
  for(int tries=0;!image&&tries<3;++tries){bool stripped=false;size_t len=strlen(ability);
   for(auto suffix:suffixes){size_t n=strlen(suffix);if(len>n&&!strcmp(ability+len-n,suffix)){ability[len-n]=0;stripped=true;break;}}
   if(!stripped)break;image=texture("abilities",ability);}}
 return image;
}
template<class Texture>
void EffectCircle(ImDrawList* dl,ImFont* font,Texture texture,const buffreader::Buff& b,float remaining,ImVec2 center,float radius,int applications){
 const char* item=nullptr;char clean[80]={};EffectName(b.name,clean,sizeof(clean),item);
 ImTextureID image=EffectTexture(texture,b);
 if(!image)return; // Unknown modifiers remain diagnostic data, not fake visible buff placeholders.
 // Only known semantic effects are red/green. Unknown polarity stays neutral.
 ImU32 edge=IM_COL32(146,166,183,255);
 if(item)edge=IM_COL32(111,194,84,255);
 if(!strcmp(b.name,"modifier_huskar_burning_spear_debuff")||!strcmp(b.name,"modifier_venomancer_poison_sting")||!strcmp(b.name,"modifier_dazzle_poison_touch"))edge=IM_COL32(235,77,62,255);
 dl->AddCircleFilled(center,radius,IM_COL32(13,18,20,245),48);
 if(image)RoundImage(dl,image,center,radius-2);
 // Missing image: neutral dark circle, no question mark overlapping the count.
 dl->AddCircle(center,radius,IM_COL32(34,43,46,255),48,3);
 float fraction=EffectFraction(b,remaining);
 if(fraction<0)dl->AddCircle(center,radius,edge,48,2.5f);
 else if(fraction>0){dl->PathArcTo(center,radius,-1.57079633f,-1.57079633f+fraction*6.2831853f,48);dl->PathStroke(edge,0,2.5f);}
 // One icon per exact modifier name. Center is stack/application count, NOT seconds.
 char count[16];if(applications>999)strcpy(count,"999+");else snprintf(count,sizeof(count),"%d",applications);
 NumericCentered(dl,font,applications>99?12.f:15.f,center,IM_COL32_WHITE,count);
 if(b.duration>=0){char sec[16];if(remaining>=3600)snprintf(sec,sizeof(sec),"%dh",(int)(remaining/3600));
  else if(remaining>=60)snprintf(sec,sizeof(sec),"%d:%02d",(int)(remaining/60),(int)remaining%60);
  else if(remaining>=0)snprintf(sec,sizeof(sec),"%.1fs",remaining);else strcpy(sec,"?");
  Text(dl,font,12,ImVec2(center.x,center.y+radius+4),IM_COL32(236,238,235,255),sec,true);}

}
struct EffectGroup {buffreader::Buff representative;int applications=0;float remaining=-2;};
inline std::vector<EffectGroup> GroupEffects(const FrameUnit& u,bool timedOnly){
 std::vector<EffectGroup> groups;
 for(auto& b:u.buffs){float remaining=buffreader::Remaining(b,u.clockRead?u.sampleTime:NAN);
  if(timedOnly&&b.duration<0)continue;if(remaining==0&&b.duration>=0)continue;
  auto it=std::find_if(groups.begin(),groups.end(),[&](const EffectGroup& g){return !strcmp(g.representative.name,b.name);});
  int count=std::max(1,b.stacks); // Non-stacking instance is one applied effect, not zero.
  if(it==groups.end()){groups.push_back({b,count,remaining});continue;}
  it->applications=std::min(1000000,it->applications+count);
  if(it->remaining==-2||remaining==-2){it->remaining=-2;continue;}
  if(it->remaining<0||remaining<0){it->remaining=-1;it->representative.duration=-1;continue;}
  // Display latest expiration among the group's layers, not a fictitious shared expiration.
  if(remaining>it->remaining){it->remaining=remaining;it->representative=b;}
 }
 std::sort(groups.begin(),groups.end(),[](const EffectGroup& a,const EffectGroup& b){return strcmp(a.representative.name,b.representative.name)<0;});
 return groups;
}
template<class Texture>
Stats Draw(const FrameUnit& u,ImDrawList* dl,ImFont* font,ImVec2 foot,ImVec2 top,int W,int H,double now,Style style,InvisTracker& tracker,Layout& layout,Texture texture){
 Stats stats;if(!dl||!u.alive||u.hp<=0||u.maxHp<=0||u.hp>u.maxHp||!std::isfinite(top.x)||!std::isfinite(top.y)||foot.x<0||foot.x>W||top.y<0||top.y>H)return stats;
 bool invis=(u.stateRead&&Bit(u.unitState,7))||(u.invisRead&&u.invis>.4f);
 auto observation=tracker.Observe(u.entityHandle,u.stateRead||u.invisRead,invis,u.alive,now);
 stats.invis=observation.active;stats.invisRemaining=observation.remaining;stats.illusion=Illusion(u);
 if(style.hp){char hp[24];snprintf(hp,sizeof(hp),"%d",u.hp);
  ImFont* hpFont=style.hpFont?style.hpFont:(font?font:ImGui::GetFont());
  // Ink bounds rather than line-box height: different fonts no longer shift the digits vertically.
  const float size=14;ImVec2 center(std::round(top.x+3+style.hpX),std::round(top.y-24+style.hpY));
  ImVec2 p=NumericOrigin(hpFont,size,hp,center);auto box=NumericInk(hpFont,size,hp);
  if(p.x+box.x>=0&&p.x+box.x+box.w<=W&&p.y+box.y>=0&&p.y+box.y+box.h<H)
   NumericCentered(dl,hpFont,size,center,IM_COL32_WHITE,hp);}
 // Marker lasts while observed invisibility stays true. Ring depletion NEVER invents a reveal.
 if(style.statuses&&observation.active){ImVec2 p(foot.x,foot.y-10);float radius=23;
  if(p.x-radius>=8&&p.x+radius<W-8&&p.y-radius>=8&&p.y+radius<H-8){
   dl->AddCircleFilled(p,radius,IM_COL32(13,21,29,220),48);dl->AddCircle(p,radius,IM_COL32(59,90,107,220),48,2);
   float remaining=observation.remaining/7.f;
   if(remaining>0){dl->PathArcTo(p,radius,-1.57079633f,-1.57079633f+remaining*6.2831853f,48);dl->PathStroke(IM_COL32(93,195,225,255),0,2.6f);}
   Text(dl,font,11,ImVec2(p.x,p.y-11),IM_COL32(154,224,246,255),"INVIS",true);char sec[12];snprintf(sec,sizeof(sec),"%.1f",observation.remaining);
   Text(dl,font,13,ImVec2(p.x,p.y+2),IM_COL32_WHITE,sec,true);
  }
 }
 std::vector<std::pair<const char*,ImU32>> tags;
 if(style.illusions&&stats.illusion)tags.emplace_back("ILLUSION",IM_COL32(194,154,242,255));
 // Public MODIFIER_STATE bit indices; positive, verified state only. No false statuses from items.
 if(style.statuses&&u.stateRead){for(auto s:{std::pair<int,const char*>{5,"STUN"},{6,"HEX"},{3,"SILENCE"},{4,"MUTE"},{0,"ROOT"},{1,"DISARM"}})if(Bit(u.unitState,s.first))tags.emplace_back(s.second,IM_COL32(242,153,135,255));}
 char extraTags[16]={};if(tags.size()>3){snprintf(extraTags,sizeof(extraTags),"+%d",(int)tags.size()-3);tags.resize(3);tags.emplace_back(extraTags,IM_COL32(160,181,198,255));}
 float pixels=std::clamp(style.pixels,28.f,60.f);float totalHeight=0;
 if(style.abilities&&u.abilN>0)totalHeight+=pixels+16;
 if(style.items&&u.inventoryRead){int n=0;bool charges=false;for(int i=0;i<u.itemN&&i<27;++i)if(u.items[i].slot>=0&&u.items[i].slot<=5&&u.items[i].icon[0]){++n;charges|=Charges(u.items[i])>0;}if(n)totalHeight+=std::min(pixels,34.f)+(charges?18.f:0.f)+8;}
 if(style.effects&&(u.buffsRead||u.buffsVisualRead)){auto groups=GroupEffects(u,style.timedOnly);int n=0;for(const auto& g:groups)if(EffectTexture(texture,g.representative))++n;if(n)totalHeight+=50;}
 if(!tags.empty())totalHeight+=28;
 // Never paint world rows through the compact fixed top HUD. Keep native HP text / observed invis marker.
 if(style.above&&top.y-54-totalHeight<style.minOverlayY)return stats;
 float anchorX=style.above?top.x:foot.x;float next=style.above?top.y-54-totalHeight:foot.y+12.f;Rect rect;
 if(!tags.empty()){float width=0;for(auto t:tags)width+=std::max(54.f,(font?font:ImGui::GetFont())->CalcTextSizeA(13,10000,0,t.first).x+16)+4;
  if(layout.Place(anchorX-width*.5f,style.above?next:foot.y-70.f,width-4,22,W,H,rect)){float x=rect.x;for(auto t:tags){float w=std::max(54.f,(font?font:ImGui::GetFont())->CalcTextSizeA(13,10000,0,t.first).x+16);Badge(dl,font,ImVec2(x,rect.y),t.first,t.second,w);x+=w+4;++stats.statuses;}if(style.above)next=rect.y+28;}}
 if(style.abilities&&u.abilN>0){int n=std::min(u.abilN,6);float width=n*(pixels+4)-4;
  if(layout.Place(anchorX-width*.5f,next,width,pixels+10,W,H,rect)){for(int i=0;i<n;++i){const auto& a=u.abil[i];Icon(dl,font,texture,"abilities",a.icon,ImVec2(rect.x+i*(pixels+4),rect.y),pixels,a.level,a.maxLevel,a.cd,a.cooldownRead,-1);++stats.abilities;}next=rect.y+pixels+16;}}
 if(style.items&&u.inventoryRead){std::vector<const ItemInfo*> items;for(int i=0;i<u.itemN&&i<27;++i)if(u.items[i].slot>=0&&u.items[i].slot<=5&&u.items[i].icon[0])items.push_back(&u.items[i]);
  if(!items.empty()){bool charges=false;for(auto item:items)if(Charges(*item)>0)charges=true;float footer=charges?18.f:0.f;float size=std::min(pixels,34.f),width=items.size()*(size+4)-4;
   if(layout.Place(anchorX-width*.5f,next,width,size+footer,W,H,rect)){for(size_t i=0;i<items.size();++i){const auto& item=*items[i];Icon(dl,font,texture,"items",item.icon,ImVec2(rect.x+i*(size+4),rect.y),size,-1,0,item.cd,item.cooldownRead,Charges(item));++stats.items;}next=rect.y+size+footer+8;}}}
 if(style.effects&&(u.buffsRead||u.buffsVisualRead)){
  auto groups=GroupEffects(u,style.timedOnly);
  groups.erase(std::remove_if(groups.begin(),groups.end(),[&](const EffectGroup& group){return !EffectTexture(texture,group.representative);}),groups.end());
  int shown=std::min((int)groups.size(),8);const float diameter=30,gap=10;
  float width=shown*(diameter+gap)-gap;
  if(shown&&layout.Place(anchorX-width*.5f,next,width,diameter+20,W,H,rect)){
   for(int i=0;i<shown;++i){auto& group=groups[i];ImVec2 center(rect.x+i*(diameter+gap)+diameter*.5f,rect.y+diameter*.5f);
    EffectCircle(dl,font,texture,group.representative,group.remaining,center,diameter*.5f,group.applications);++stats.effects;}
  }
 }
 return stats;
}
}
