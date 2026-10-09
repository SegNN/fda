#include "fog_memory.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){fogmemory::Tracker t;
 t.Begin(100);t.Observe(0x8001,true,true,false,{999,999,0});assert(t.End().empty()); // Never seen, no invented position.
 t.Begin(101);t.Observe(0x8001,true,true,true,{1,2,3},"npc_dota_hero_huskar");assert(t.End().empty());
 t.Begin(102);t.Observe(0x8001,true,true,false,{900,900,900},"npc_dota_hero_juggernaut");auto v=t.End();assert(v.size()==1&&v[0].remaining==10&&v[0].pos.x==1&&v[0].portrait=="huskar");
 t.Begin(107);t.Observe(0x8001,true,true,false,{900,900,900});v=t.End();assert(v[0].remaining==5&&v[0].pos.y==2);
 t.Begin(107);t.Observe(0x8001,true,true,false,{});assert(t.End()[0].remaining==5); // Pause.
 t.Begin(112);t.Observe(0x8001,true,true,false,{});assert(t.End().empty());
 t.Begin(113);t.Observe(0x8001,true,true,true,{5,6,7});assert(t.End().empty());
 t.Begin(114);v=t.End();assert(v.size()==1&&v[0].remaining==10&&v[0].pos.x==5); // Removed snapshot.
 t.Begin(115);t.Observe(0x8001,true,true,true,{9,9,9});assert(t.End().empty()); // Visible immediately cancels marker.
 t.Begin(116);t.Observe(0x8001,true,false,false,{});assert(t.End().empty()); // Unknown != hidden.
 t.Begin(117);t.Observe(0x8001,true,true,true,{});t.End();t.Begin(118);t.Observe(0x8001,false,true,false,{});assert(t.End().empty());
 t.Begin(120);t.Observe(0x8001,true,true,true,{1,2,3});t.End();t.Begin(121);t.Observe(0xc001,true,true,true,{4,5,6});v=t.End();assert(v.size()==1&&v[0].handle==0x8001); // Distinct serial.
 t.Begin(0);assert(t.End().empty()); // New game/time reset.
 t.Begin(1);t.Observe(1,true,true,true,{NAN,0,0});assert(t.End().empty());
 assert(fogmemory::PortraitKey("npc_dota_hero_huskar")=="huskar"&&fogmemory::PortraitKey("Huskar")=="huskar");
 assert(fogmemory::PortraitKey("../huskar").empty()&&fogmemory::PortraitKey("x/y").empty());
 t.Reset();t.Begin(200);t.Observe(1,true,true,true,{1,2,3},"npc_dota_hero_huskar");t.Observe(2,true,true,true,{4,5,6},"npc_dota_hero_juggernaut");t.End();
 t.Begin(201);auto avatars=t.End();assert(avatars.size()==2);for(auto m:avatars)assert(m.portrait==(m.handle==1?"huskar":"juggernaut"));
 t.Begin(202);t.Observe(1,true,true,true,{7,8,9},"npc_dota_hero_lina");t.End();t.Begin(203);avatars=t.End();for(auto m:avatars)if(m.handle==1)assert(m.portrait=="lina");
 assert(fogmemory::PortraitKey(std::string(81,'x').c_str()).empty());
 t.Reset();assert(t.End().empty());std::cout<<"PASS fog memory: ten seconds, pause, reappearance, absent snapshots, unknown, death, serials, new game, finite positions.\n";
}
