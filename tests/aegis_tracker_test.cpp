#include "../src/aegis_tracker.h"
#include <cassert>
#include <cstdio>
int main(){aegis::Tracker t;const uintptr_t hero=10,item=20;
 auto check=[&](float now,bool present,bool absent,float expiry,uint32_t serial=1,bool alive=true){t.BeginFrame(now);return t.Observe(hero,item,serial,present,absent,expiry,now,alive);};
 auto r=check(100,true,true,-1);assert(r.visible&&r.estimated&&r.seconds==300);
 r=check(160,true,true,-1);assert(r.seconds==240); // no frame-by-frame rearm
 r=check(160,true,true,-1);assert(r.seconds==240); // pause
 r=check(180,false,false,-1);assert(r.visible&&r.estimated&&r.seconds==220);
 r=check(200,true,true,310);assert(r.visible&&!r.estimated&&r.seconds==110);
 r=check(220,true,true,-1);assert(!r.estimated&&r.seconds==90);
 r=check(240,false,true,-1);assert(!r.visible);
 r=check(250,true,true,-1);assert(!r.visible); // stale same handle after disappearance
 r=check(260,true,true,-1,2);assert(r.seconds==300);
 r=check(560,true,true,-1,2);assert(!r.visible&&r.seconds==0);
 r=check(600,true,true,-1,2);assert(!r.visible);
 r=check(600,true,true,-1,3);assert(r.visible&&r.seconds==300);
 r=check(10,true,true,-1,3);assert(r.seconds==300); // new match clock
 r=check(20,true,true,19,3);assert(!r.visible&&!r.estimated);
 r=check(30,true,true,300,3);assert(!r.visible); // late end time must not revive expired instance
 t.Reset();r=check(100,true,true,400);assert(!r.estimated&&r.seconds==300);
 r=check(200,true,true,400,1,false);assert(!r.visible); // death before inventory removal
 r=check(210,true,true,400,1,true);assert(!r.visible); // resurrection cannot rearm consumed item
 r=check(220,true,true,-1,2,true);assert(r.visible&&r.seconds==300);
 t.Reset();r=check(100,true,true,-1,1,false);assert(!r.visible); // initially observed dead owner
 t.Reset();t.BeginFrame(100);r=t.Observe(11,21,1,true,true,-1,100);assert(r.seconds==300);
 t.BeginFrame(200);r=t.Observe(10,20,1,true,true,400,200);assert(!r.estimated&&r.seconds==200);
 puts("Aegis: 22 lifecycle assertions passed, including death, expiration, pause and stale reads.");
}
