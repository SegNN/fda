#pragma once
#include "kill_stealer_core.h"
namespace armletcore {
// Read-only observation -> at most one desired toggle per tick. No engine API here.
enum class Phase {Idle,WaitOff,WaitOn,Recover,Fault};
enum class Action {None,TurnOff,TurnOn};
struct Input {bool allowed=false,on=false,ready=false,allowTurnOff=true,effectRead=false,effectOn=false;float now=0,threshold=350;int hp=0;uint32_t hero=0,item=0;};
inline const char* DangerousName(const std::vector<buffreader::Buff>& buffs){
 const char* words[]={"poison","burning_spear_debuff","burning","liquid_fire","fire_spirit","bleed","rupture","doom","venom","acid_spray","macropyre","radiance","pudge_rot","life_break_charge","heartstopper","maledict","battle_hunger","ice_blast","shadow_strike","fatal_bonds","urn_damage","spirit_vessel_damage","ion_shell","flame_guard"};
 for(const auto& b:buffs){if(!strcmp(b.name,"modifier_huskar_burning_spear")||!strcmp(b.name,"modifier_huskar_burning_spear_counter")||!strcmp(b.name,"modifier_huskar_burning_spear_self"))continue;for(const char* w:words)if(strstr(b.name,w))return b.name;}return nullptr;
}
inline bool Dangerous(const std::vector<buffreader::Buff>& buffs){return DangerousName(buffs)!=nullptr;}
class Machine {
 Phase phase=Phase::Idle;float previous=-1,started=0,last=-1000;bool armed=true;
 uint32_t hero=0,item=0;const char* fault=nullptr;
public:
 const char* FaultReason()const{return fault?fault:"none";}
 Phase State()const{return phase;}
 bool Armed()const{return armed;}
 float CooldownRemaining(float now)const{return std::isfinite(now)?std::max(0.f,2.f-(now-last)):0.f;}
 bool Pending()const{return phase==Phase::WaitOff||phase==Phase::WaitOn||phase==Phase::Recover;}
 void Reset(){*this=Machine{};}
 void Cancel(const char* why="Cycle canceled"){if(Pending())Failed(why);}
 void Failed(const char* why="Input delivery failed"){if(phase!=Phase::Fault)fault=why;phase=Phase::Fault;}
 Action Tick(const Input& x){
  if(!std::isfinite(x.now)||!std::isfinite(x.threshold)||x.threshold<50||x.threshold>550||x.hp<=0||!x.hero||!x.item){Cancel("Invalid time / threshold / HP / identity");return Action::None;}
  if(previous>=0&&(x.hero!=hero||x.item!=item||x.now<previous||x.now-previous>.5f)){
   // Pending toggles are never delivered late to a changed hero/item/match.
   if(Pending())Failed("Hero/item changed or clock gap/rollback");else Reset();
  }
  previous=x.now;hero=x.hero;item=x.item;
  if(!x.allowed){Cancel("Input not allowed");return Action::None;}
  if(phase==Phase::Fault)return Action::None;
  if(phase==Phase::WaitOff){
   if(x.now-started>.7f){Failed("OFF observation timed out");return Action::None;}
   if(!x.on&&x.ready){phase=Phase::WaitOn;started=x.now;return Action::TurnOn;}return Action::None;
  }
  if(phase==Phase::WaitOn){
   if(x.now-started>.7f){Failed("ON / active effect observation timed out");return Action::None;}
   if(x.on&&x.effectRead&&x.effectOn){phase=Phase::Recover;started=x.now;}return Action::None;
  }
  if(phase==Phase::Recover){
   if(!x.on){Failed("Armlet became OFF during recovery");return Action::None;}
   if(x.now-started>=1.f){if(!x.effectRead||!x.effectOn){Failed("Active Armlet effect not demonstrated");return Action::None;}phase=Phase::Idle;armed=true;}
   return Action::None;
  }
  if(x.hp>x.threshold+100)armed=true;
  // Damage-risk veto applies ONLY to a new OFF; never cancels an observed OFF -> ON.
  if(x.on&&!x.allowTurnOff)return Action::None;
  if(x.hp>x.threshold||!x.ready||x.now-last<2.f||!armed)return Action::None;
  last=x.now;armed=false;started=x.now;
  phase=x.on?Phase::WaitOff:Phase::WaitOn;
  return x.on?Action::TurnOff:Action::TurnOn;
 }
};
}
