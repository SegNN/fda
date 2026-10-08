#pragma once
#include "kill_stealer_core.h"
namespace killhelpercore {
struct Model {const char* hero;const char* skill;const char* label;int type,levels;float damage[4],range[4];};
inline const Model catalog[]={
#include "kill_helper_catalog.inc"
};
inline const Model* Find(const char* hero,const char* skill){for(const auto& m:catalog)if(!strcmp(m.hero,hero)&&!strcmp(m.skill,skill))return &m;return nullptr;}
inline bool Values(const Model* m,int level,float& damage,float& range){if(!m||level<1||level>m->levels)return false;damage=m->damage[level-1];range=m->range[level-1];return true;}
enum class Status {Unknown,Blocked,NotReady,Mana,Range,Short,Enough};
struct Result {bool numeric=false;float damage=0,delta=0;Status status=Status::Unknown;};
// Enough is only conditional arithmetic, NEVER a certified kill or an input command.
inline Result Evaluate(const killcore::Input& x){
 Result r;
 if(x.hp<=0||!std::isfinite(x.rawDamage)||x.rawDamage<=0||x.rawDamage>100000||!std::isfinite(x.margin)||x.margin<0||x.margin>1000)return r;
 float mult=1;
 if(x.type==0){if(!x.resistanceRead||!std::isfinite(x.resistance)||x.resistance<-.99f||x.resistance>1)return r;mult=1-x.resistance;}
 else if(x.type==1){if(!x.resistanceRead||!std::isfinite(x.resistance)||std::abs(x.resistance)>10000)return r;mult=1-(.06f*x.resistance)/(1+.06f*std::abs(x.resistance));}
 else if(x.type!=2)return r;
 r.numeric=true;r.damage=x.rawDamage*mult;r.delta=r.damage-x.hp;
 if(!x.visible||!x.alive||x.illusion)return r;
 if(x.protectedTarget||(x.stateRead&&((x.localState&killcore::CannotCast)||(x.targetState&killcore::CannotTarget)))){r.status=Status::Blocked;return r;}
 if(!x.ready){r.status=Status::NotReady;return r;}
 if(x.cost<0||x.mana<0)return r;
 if(x.mana<x.cost){r.status=Status::Mana;return r;}
 if(!std::isfinite(x.distance)||x.distance<0||!std::isfinite(x.range)||x.range<=0||x.range>2500)return r;
 if(x.distance>x.range){r.status=Status::Range;return r;}
 if(r.delta<x.margin){r.status=Status::Short;return r;}
 if(!x.stateRead||!x.buffsRead)return r;
 r.status=Status::Enough;return r;
}
}
