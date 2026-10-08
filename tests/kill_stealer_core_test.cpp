#include "../src/kill_stealer_core.h"
#include <cassert>
#include <iostream>
int main(){killcore::Input x; x.visible=x.alive=x.buffsRead=x.stateRead=x.resistanceRead=x.ready=true;x.hp=200;x.mana=300;x.cost=100;x.range=600;x.distance=500;x.rawDamage=400;x.resistance=.25;x.margin=30;
 assert(killcore::Lethal(x));x.hp=280;assert(!killcore::Lethal(x));x.hp=200;x.buffsRead=false;assert(!killcore::Lethal(x));x.buffsRead=true;
 for(int i:{8,9,14,33,37,39,56}){x.targetState=killcore::Bit(i);assert(!killcore::Lethal(x));}x.targetState=0;
 x.localState=killcore::Bit(3);assert(!killcore::Lethal(x));x.localState=0;
 x.protectedTarget=true;assert(!killcore::Lethal(x));x.protectedTarget=false;x.distance=601;assert(!killcore::Lethal(x));x.distance=500;
 x.ready=false;assert(!killcore::Lethal(x));x.ready=true;x.mana=50;assert(!killcore::Lethal(x));
 assert(killcore::Blocking("modifier_item_sphere_target"));assert(killcore::Blocking("modifier_dazzle_shallow_grave"));assert(!killcore::Blocking("modifier_tango_heal"));
 std::cout<<"PASS kill profile damage, resistance, immunity, visibility, buffs, readiness, mana and blocking modifiers\n";
}
