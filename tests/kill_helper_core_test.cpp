#include "../src/kill_helper_core.h"
#include <cassert>
#include <limits>
#include <iostream>
using namespace killhelpercore;
int main(){
 assert(sizeof(catalog)/sizeof(*catalog)==17);
 for(const auto& m:catalog){assert(m.levels>0&&m.levels<=4);for(int i=1;i<=m.levels;++i){float d=0,r=0;assert(Values(&m,i,d,r)&&d>0&&r>0);}float d=9,r=9;assert(!Values(&m,0,d,r)&&!Values(&m,m.levels+1,d,r));}
 float d=0,r=0;auto m=Find("npc_dota_hero_lina","lina_laguna_blade");assert(Values(m,1,d,r)&&d==400&&r==750);assert(!Find("npc_dota_hero_pudge","pudge_dismember"));assert(!Find("npc_dota_hero_lion","lina_laguna_blade"));
 killcore::Input x;x.visible=x.alive=x.buffsRead=x.stateRead=x.resistanceRead=x.ready=true;x.hp=280;x.mana=500;x.cost=150;x.rawDamage=400;x.range=750;x.distance=500;x.resistance=.25;x.margin=30;
 auto e=Evaluate(x);assert(e.numeric&&e.damage==300&&e.delta==20&&e.status==Status::Short);
 x.margin=20;assert(Evaluate(x).status==Status::Enough);
 x.buffsRead=false;assert(Evaluate(x).status==Status::Unknown);x.buffsRead=true;
 x.stateRead=false;assert(Evaluate(x).status==Status::Unknown);x.stateRead=true;
 x.protectedTarget=true;assert(Evaluate(x).status==Status::Blocked);x.protectedTarget=false;
 x.targetState=killcore::Bit(8);assert(Evaluate(x).status==Status::Blocked);x.targetState=0;
 x.localState=killcore::Bit(3);assert(Evaluate(x).status==Status::Blocked);x.localState=0;
 x.ready=false;assert(Evaluate(x).status==Status::NotReady);x.ready=true;
 x.mana=100;assert(Evaluate(x).status==Status::Mana);x.mana=500;
 x.distance=751;assert(Evaluate(x).status==Status::Range);x.distance=500;
 x.resistanceRead=false;assert(!Evaluate(x).numeric);x.type=2;assert(Evaluate(x).damage==400);
 x.type=1;x.resistanceRead=true;x.resistance=10;assert(std::abs(Evaluate(x).damage-250)<.01);
 x.resistance=-10;assert(std::abs(Evaluate(x).damage-550)<.01);
 x.resistance=std::numeric_limits<float>::quiet_NaN();assert(!Evaluate(x).numeric);
 x.type=3;assert(!Evaluate(x).numeric);x.type=2;x.hp=0;assert(!Evaluate(x).numeric);x.hp=280;
 x.range=0;assert(Evaluate(x).status==Status::Unknown);
 std::cout<<"PASS helper catalog levels and formulas; HP/margin, unknowns, protection, ready, mana, range and invalid input guards\n";
}
