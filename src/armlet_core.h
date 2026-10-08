#pragma once
#include "kill_stealer_core.h"
namespace armletcore {
// Read-only observation -> at most one desired toggle per tick. No engine API here.
enum class Phase {Idle,WaitOff,WaitOn,Recover,Fault};
enum class Action {None,TurnOff,TurnOn};
struct Input {bool allowed=false,on=false,ready=false;float now=0,threshold=350;int hp=0;uint32_t hero=0,item=0;};
inline bool Dangerous(const std::vector<buffreader::Buff>& buffs){
 const char* words[]={"poison","burning_spear_debuff","burning","liquid_fire","fire_spirit","bleed","rupture","doom","venom","acid_spray","macropyre","radiance","pudge_rot","life_break_charge","heartstopper","maledict","battle_hunger","ice_blast","shadow_strike","fatal_bonds","urn_damage","spirit_vessel_damage","ion_shell","flame_guard"};
 for(const auto& b:buffs){if(!strcmp(b.name,"modifier_huskar_burning_spear")||!strcmp(b.name,"modifier_huskar_burning_spear_counter"))continue;for(const char* w:words)if(strstr(b.name,w))return true;}return false;
}
class Machine {
 Phase phase=Phase::Idle;float previous=-1,started=0,last=-1000;int initialHP=0;bool armed=true;
 uint32_t hero=0,item=0;
public:
 Phase State()const{return phase;}
 bool Pending()const{return phase==Phase::WaitOff||phase==Phase::WaitOn||phase==Phase::Recover;}
 void Reset(){*this=Machine{};}
 void Cancel(){if(Pending())phase=Phase::Fault;}
 void Failed(){phase=Phase::Fault;}
 Action Tick(const Input& x){
  if(!std::isfinite(x.now)||!std::isfinite(x.threshold)||x.threshold<50||x.threshold>550||x.hp<=0||!x.hero||!x.item){Cancel();return Action::None;}
  if(previous>=0&&(x.hero!=hero||x.item!=item||x.now<previous||x.now-previous>.5f)){
   // Pending toggles are never delivered late to a changed hero/item/match.
   if(Pending())phase=Phase::Fault;else Reset();
  }
  previous=x.now;hero=x.hero;item=x.item;
  if(!x.allowed){Cancel();return Action::None;}
  if(phase==Phase::Fault)return Action::None;
  if(phase==Phase::WaitOff){
   if(x.now-started>.7f){phase=Phase::Fault;return Action::None;}
   if(!x.on&&x.ready&&x.now-started>=.1f){phase=Phase::WaitOn;started=x.now;return Action::TurnOn;}return Action::None;
  }
  if(phase==Phase::WaitOn){
   if(x.now-started>.7f){phase=Phase::Fault;return Action::None;}
   if(x.on){phase=Phase::Recover;started=x.now;}return Action::None;
  }
  if(phase==Phase::Recover){
   if(!x.on){phase=Phase::Fault;return Action::None;}
   if(x.now-started>=1.f){if(x.hp<=initialHP){phase=Phase::Fault;return Action::None;}phase=Phase::Idle;}
   return Action::None;
  }
  if(x.hp>x.threshold+100)armed=true;
  if(x.hp>x.threshold||!x.ready||x.now-last<2.f||!armed)return Action::None;
  last=x.now;initialHP=x.hp;armed=false;started=x.now;
  phase=x.on?Phase::WaitOff:Phase::WaitOn;
  return x.on?Action::TurnOff:Action::TurnOn;
 }
};
}
