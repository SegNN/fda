#include "hero_info.h"
#include "ability_max_levels.h"
#include <cassert>
#include <iostream>
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1280,720};io.DeltaTime=1.f/60;
 io.Fonts->AddFontDefault();unsigned char* px;int w,h;io.Fonts->GetTexDataAsRGBA32(&px,&w,&h);io.Fonts->SetTexID((ImTextureID)1);ImGui::NewFrame();
 auto* dl=ImGui::GetBackgroundDrawList();heroinfo::InvisTracker tracker;heroinfo::Layout layout;
 FrameUnit u;u.kind=UnitKind::Hero;u.entityHandle=0x8001;u.alive=true;u.hp=582;u.maxHp=1000;u.level=6;u.clockRead=true;u.sampleTime=101;
 u.illusionRead=true;u.illusion=true;u.stateRead=true;u.unitState=uint64_t(1)<<5;u.inventoryRead=true;u.itemN=1;u.items[0].slot=0;strcpy(u.items[0].icon,"armlet");u.items[0].cooldownRead=true;u.items[0].cd=0;
 u.abilN=1;strcpy(u.abil[0].icon,"huskar_inner_fire");u.abil[0].level=2;u.abil[0].maxLevel=4;u.abil[0].cooldownRead=true;u.abil[0].cd=3;
 u.buffsRead=true;buffreader::Buff flask;strcpy(flask.name,"modifier_flask_healing");flask.duration=10;flask.created=100;flask.expires=110;u.buffs={flask};
 assert(heroinfo::Charges(u.items[0])==-1);u.items[0].charges=0;assert(heroinfo::Charges(u.items[0])==-1);u.items[0].charges=3;assert(heroinfo::Charges(u.items[0])==3);u.items[0].charges=0;
 heroinfo::Style style;auto texture=[](const char*,const char*)->ImTextureID{return 2;};int before=dl->VtxBuffer.Size;
 auto stats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,180},1280,720,0,style,tracker,layout,texture);
 assert(stats.items==1&&stats.abilities==1&&stats.effects==1&&stats.illusion&&stats.statuses==2&&!stats.invis&&dl->VtxBuffer.Size>before);
 for(auto& r:layout.occupied){assert(r.x>=8&&r.y>=8&&r.x+r.w<=1272&&r.y+r.h<=712);}
 // ESP19 above-bar rows must not overlap the fixed top HUD region; no teleporting onto another hero.
 auto protectedStyle=style;protectedStyle.above=true;protectedStyle.minOverlayY=200;heroinfo::Layout protectedLayout;
 auto protectedStats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,236},1280,720,0,protectedStyle,tracker,protectedLayout,texture);
 assert(protectedStats.abilities==0&&protectedStats.items==0&&protectedStats.effects==0&&protectedLayout.occupied.empty());
 protectedStats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,620},{600,540},1280,720,0,protectedStyle,tracker,protectedLayout,texture);assert(protectedStats.abilities==1&&protectedStats.items==1);
 // Repeated anchors, including close heroes, never get displaced by occupied rows.
 heroinfo::Rect first,second;heroinfo::Layout close;
 assert(close.Place(500,400,140,38,1280,720,first));
 assert(close.Place(500,400,140,38,1280,720,second));assert(first.x==second.x&&first.y==second.y);
 assert(close.Place(-30,690,140,38,1280,720,second)&&second.x==-30&&second.y==690);
 assert(heroinfo::EffectFraction(flask,5)==.5f&&heroinfo::EffectFraction(flask,-2)==-1);

 assert(heroinfo::HpOffset(5,6,false)==5&&heroinfo::HpOffset(5,6,true)==11);
 // No fake circles for missing textures: all effect metadata still remains in the snapshot.
 auto missing=[](const char*,const char*)->ImTextureID{return 0;};heroinfo::Layout missingLayout;
 auto missingStats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,180},1280,720,0,style,tracker,missingLayout,missing);
 assert(missingStats.effects==0&&u.buffs.size()==1);
 // Exact published maxima only; no universal guess for unknown spells.
 assert(PublishedAbilityMax("huskar_inner_fire")==4&&PublishedAbilityMax("huskar_life_break")==3&&PublishedAbilityMax("unknown")==0);
 // Duplicate applications group by exact name. Expiry uses latest layer; stacks in center.
 FrameUnit effects;effects.clockRead=true;effects.sampleTime=101;effects.buffs={flask,flask};
 effects.buffs[0].stacks=2;effects.buffs[1].stacks=3;effects.buffs[1].expires=106;
 auto groups=heroinfo::GroupEffects(effects,true);assert(groups.size()==1&&groups[0].applications==5&&groups[0].remaining==9);
 effects.clockRead=false;groups=heroinfo::GroupEffects(effects,true);assert(groups[0].remaining==-2);
 effects.clockRead=true;effects.sampleTime=111;assert(heroinfo::GroupEffects(effects,true).empty());
 auto box=heroinfo::NumericInk(ImGui::GetFont(),14,"440");auto origin=heroinfo::NumericOrigin(ImGui::GetFont(),14,"440",{129,98});
 assert(fabsf(origin.x+box.x+box.w*.5f-129)<=.51f&&fabsf(origin.y+box.y+box.h*.5f-98)<=.51f);
 layout.occupied.clear();u.unitState=uint64_t(1)<<7;stats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,180},1280,720,1,style,tracker,layout,texture);assert(stats.invis&&stats.invisRemaining==7);
 // Must observe every frame; a visibility gap >2s starts a new observed interval, not a cast timer.
 for(int i=1;i<=70;++i)tracker.Observe(u.entityHandle,true,true,true,1+i*.1);
 auto obs=tracker.Observe(u.entityHandle,true,true,true,8.1);assert(obs.active&&obs.remaining==0); // no fake reveal at 7 seconds
 assert(!tracker.Observe(u.entityHandle,false,false,true,8.2).known); // unknown is not "visible"
 assert(!tracker.Observe(u.entityHandle,true,false,true,8.3).active);
 obs=tracker.Observe(u.entityHandle,true,true,true,8.4);assert(obs.remaining==7);
 assert(!tracker.Observe(u.entityHandle,true,true,false,8.5).active);
 obs=tracker.Observe(u.entityHandle+0x4000,true,true,true,8.6);assert(obs.remaining==7); // full serial, not address cache
 u.illusionRead=false;u.illusion=true;u.buffsRead=false;u.buffs.clear();u.inventoryRead=false;u.abilN=0;u.stateRead=u.invisRead=false;layout.occupied.clear();
 stats=heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,180},1280,720,9,style,tracker,layout,texture);
 assert(!stats.illusion&&!stats.invis&&stats.items==0&&stats.effects==0&&stats.statuses==0);
 buffreader::Buff illusion;strcpy(illusion.name,"modifier_illusion");u.buffsRead=true;u.buffs={illusion};assert(heroinfo::Illusion(u));
 u.buffs.clear();u.illusionRead=true;u.illusion=false;style.hp=style.abilities=style.items=style.effects=style.statuses=style.illusions=false;layout.occupied.clear();before=dl->VtxBuffer.Size;
 heroinfo::Draw(u,dl,ImGui::GetFont(),{600,350},{600,180},1280,720,10,style,tracker,layout,texture);assert(dl->VtxBuffer.Size==before); // no forced bar/box/label
 ImGui::EndFrame();ImGui::DestroyContext();std::cout<<"PASS HUD8 actual HeroInfo ImGui geometry: native HP text/status/illusion/items/ability levels/Flask, stable hero-local layout, observed-7s invis ring, unknown/dead/full-serial guards and all-off mode. Not a GPU/live-game test.\n";
}
