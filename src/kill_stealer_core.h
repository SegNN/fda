#pragma once
#include "buff_reader.h"
#include <cmath>
#include <cstring>
namespace killcore {
inline bool Blocking(const char* n){
 const char* words[]={"shallow_grave","borrowed_time","false_promise","aeon_disk","sphere","lotus_orb_active","counterspell","reflection","invulner","bkb_immune","black_king_bar_immune","reincarnation","spell_immunity","spell_block","spell_shield"};
 for(const char* word:words)if(strstr(n,word))return true;return false;
}
inline bool Protected(const std::vector<buffreader::Buff>& buffs){for(const auto& b:buffs)if(Blocking(b.name))return true;return false;}
constexpr uint64_t Bit(int n){return uint64_t(1)<<n;}
constexpr uint64_t CannotCast=Bit(3)|Bit(5)|Bit(6)|Bit(11)|Bit(15)|Bit(19)|Bit(20)|Bit(33)|Bit(47)|Bit(48);
constexpr uint64_t CannotTarget=Bit(8)|Bit(9)|Bit(14)|Bit(33)|Bit(37)|Bit(39)|Bit(56);
struct Input {bool visible=false,alive=false,illusion=false,buffsRead=false,stateRead=false,resistanceRead=false,ready=false;int hp=0,mana=0,cost=0,type=0;float distance=0,range=0,rawDamage=0,resistance=0,margin=0;uint64_t localState=0,targetState=0;bool protectedTarget=false;};
inline bool Lethal(const Input& x){
 if(!x.visible||!x.alive||x.illusion||!x.buffsRead||!x.stateRead||!x.ready||x.protectedTarget||x.hp<=0||x.cost<0||x.mana<x.cost||
    (x.localState&CannotCast)||(x.targetState&CannotTarget))return false;
 if(!std::isfinite(x.distance)||!std::isfinite(x.range)||x.distance<0||x.range<=0||x.range>2500||x.distance>x.range||
    !std::isfinite(x.rawDamage)||x.rawDamage<=0||x.rawDamage>100000||!std::isfinite(x.margin)||x.margin<0||x.margin>1000)return false;
 float damage=x.rawDamage;
 if(x.type==0){if(!x.resistanceRead||!std::isfinite(x.resistance)||x.resistance<-.99f||x.resistance>=1.f)return false;damage*=1-x.resistance;}
 else if(x.type==1){if(!x.resistanceRead||!std::isfinite(x.resistance)||std::abs(x.resistance)>10000)return false;damage*=1-(.06f*x.resistance)/(1+.06f*std::abs(x.resistance));}
 else if(x.type!=2)return false;
 return damage-x.margin>=x.hp;
}
}
