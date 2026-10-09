#pragma once
#include <cstdint>
#include <cmath>
namespace combocore {
enum class Phase {Idle,Aiming,AwaitCast,Done,Fault};
enum class Action {None,Aim,Cast};
struct Input {bool held=false,valid=false,spellExists=false,ready=false,inRange=false;uint32_t hero=0,target=0,spell=0;double now=0;float cooldown=0;int mana=0,cost=0;};
class Machine {
 Phase phase=Phase::Idle;int step=0;uint32_t hero=0,target=0,spell=0;double at=0,last=-1;const char* reason="Idle";
 Action Fail(const char* why){phase=Phase::Fault;reason=why;return Action::None;}
public:
 Phase State()const{return phase;}int Step()const{return step;}uint32_t Target()const{return target;}const char* Status()const{return reason;}
 void Reset(){phase=Phase::Idle;step=0;hero=target=spell=0;last=-1;reason="Idle";}
 void Stop(const char* why){if(phase!=Phase::Fault)Fail(why);}
 void Submitted(bool ok){if(!ok)Fail("Input delivery failed; release binding");}
 Action Tick(const Input& x,int count){
  if(!x.held){Reset();reason="Release / idle";return Action::None;}
  if(phase==Phase::Done||phase==Phase::Fault)return Action::None;
  if(!x.valid||!x.hero||!x.target||!std::isfinite(x.now))return Fail("Context unavailable; release binding");
  if(x.now<0)return Fail("Invalid clock; release binding");
  if(last>=0&&x.now<last)return Fail("Clock reset; release binding");last=x.now;
  if(count<1||count>8)return Fail("Invalid combo plan");
  if(phase==Phase::Idle&&!hero){hero=x.hero;target=x.target;step=0;}
  if(hero!=x.hero||target!=x.target)return Fail("Hero/target identity changed; release binding");
  if(!x.spellExists||!x.spell||!std::isfinite(x.cooldown)||x.cooldown<0||x.cost<0)return Fail("Spell unreadable or identity unavailable");
  if(phase==Phase::AwaitCast){
   if(x.spell!=spell)return Fail("Spell replaced before cast acknowledgement");
   // Phase start/mana loss do not prove a cast: wait for this exact spell's cooldown.
   if(x.cooldown>.05f){++step;phase=step>=count?Phase::Done:Phase::Idle;reason=phase==Phase::Done?"Combo acknowledged; release binding":"Cast acknowledged; next spell";return Action::None;}
   if(x.now-at>2.)return Fail("No cast acknowledgement within 2s; release binding");
   return Action::None;
  }
  if(!x.ready||x.cooldown>.01f||x.mana<0||x.mana<x.cost)return Fail("Spell not ready / insufficient mana");
  if(!x.inRange)return Fail("Target outside conservative range");
  if(phase==Phase::Idle){spell=x.spell;at=x.now;phase=Phase::Aiming;reason="Aiming; wait before quickcast";return Action::Aim;}
  if(x.spell!=spell)return Fail("Spell changed while aiming");
  if(x.now-at<.05)return Action::None;
  if(x.now-at>.35)return Fail("Aim delay too long; release binding");
  phase=Phase::AwaitCast;at=x.now;reason="Quickcast submitted; waiting cooldown confirmation";return Action::Cast;
 }
};
}
