#include "../src/map_event_tracker.h"
#include <cassert>
#include <limits>
#include <iostream>
using namespace mapeventcore;
int main(){
 Settings cfg;cfg.wards=true;Tracker t;
 Observation sp{1,Kind::Spawner,5,5,0,120,500,500,true};
 assert(t.Update(true,1,0,100,{sp},cfg).empty()); // silent attach
 auto events=t.Update(true,1,0,110,{sp},cfg);assert(events.empty()); // gap causes silent baseline
 events=t.Update(true,1,0,111,{sp},cfg);assert(events.empty()); // already within lead at baseline
 sp.next=180;
 assert(t.Update(true,1,0,112,{sp},cfg).empty());
 for(float now=115;now<=170;now+=5){events=t.Update(true,1,0,now,{sp},cfg);if(now==170)assert(events.size()==1&&events[0].kind==EventKind::RuneSoon);else assert(events.empty());}
 assert(t.Update(true,1,0,170,{sp},cfg).empty()); // paused and dedup
 assert(t.Update(true,1,0,171,{sp},cfg).empty());
 for(float now=175;now<=180;now+=5)t.Update(true,1,0,now,{sp},cfg);
 sp.stamp=180;sp.next=240;
 events=t.Update(true,1,0,181,{sp},cfg);assert(events.size()==1&&events[0].kind==EventKind::RuneSpawnObserved);
 assert(t.Update(true,1,0,182,{sp},cfg).empty());
 Observation rune{2,Kind::Rune,7,-1,183,-1,-100,900,true};
 events=t.Update(true,1,0,183,{sp,rune},cfg);assert(events.size()==1&&events[0].kind==EventKind::RuneDiscovered);
 t.Update(true,1,0,184,{sp},cfg);
 assert(t.Update(true,1,0,185,{sp,rune},cfg).empty()); // visibility flicker
 rune.handle=3;events=t.Update(true,1,0,186,{sp,rune},cfg);assert(events.size()==1); // recycled serial
 cfg.runes=false;t.Update(true,1,0,187,{sp,rune},cfg);cfg.runes=true;rune.handle=4;
 assert(t.Update(true,1,0,188,{sp,rune},cfg).empty()); // enabling doesn't flood old entities
 Observation ward{5,Kind::EnemyWard,-1,-1,-1,-1,100,100,true};
 events=t.Update(true,1,0,189,{sp,rune,ward},cfg);assert(events.size()==1&&events[0].kind==EventKind::WardDiscovered);
 Observation roshan{6,Kind::Roshan,-1,-1,-1,-1,900,500,true};
 assert(t.Update(true,1,0,190,{sp,rune,ward,roshan},cfg).empty());
 assert(t.Update(true,1,0,191,{sp,rune,ward},cfg).empty()); // disappearance isn't death
 roshan.alive=false;events=t.Update(true,1,0,192,{roshan},cfg);assert(events.size()==1&&events[0].kind==EventKind::RoshanDead);
 roshan.alive=true;events=t.Update(true,1,0,193,{roshan},cfg);assert(events.size()==1&&events[0].kind==EventKind::RoshanAlive);
 assert(t.Update(true,2,0,194,{sp,rune,ward,roshan},cfg).empty()); // new match
 assert(t.Update(true,2,0,100,{sp,rune},cfg).empty()); // rollback; invalid future rune ignored
 assert(t.Update(false,2,0,101,{},cfg).empty());
 assert(t.Update(true,2,0,102,{sp},cfg).empty()); // invalid gap rebaseline
 rune.stamp=102;rune.x=std::numeric_limits<float>::quiet_NaN();
 assert(t.Update(true,2,0,103,{rune},cfg).empty()); // invalid data
 rune.x=0;rune.type=999;assert(t.Update(true,2,0,104,{rune},cfg).empty());
 sp.next=105;sp.stamp=105;t.Update(true,2,0,105,{sp},cfg);
 assert(t.Update(true,2,0,106,{sp},cfg).empty()); // past schedule doesn't confirm spawn
 Tracker d;sp.stamp=100;sp.next=120;d.Update(true,1,0,100,{sp},cfg);
 sp.stamp=101;rune={2,Kind::Rune,5,-1,101,-1,500,500,true};
 events=d.Update(true,1,0,101,{rune,sp},cfg);assert(events.size()==1&&events[0].kind==EventKind::RuneSpawnObserved);
 std::cout<<"PASS: map events baseline, dedup, pause, rollback, re-enable, serial, unknown data, Roshan and visibility cases\n";
}
