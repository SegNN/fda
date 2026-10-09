#include "../src/npc_visibility.h"
#include "../src/compact_top.h"
#include <map>
#include <cassert>
#include <iostream>
struct Reader {std::map<uintptr_t,uint64_t> words;uintptr_t torn=0;int reads=0;bool Read(uintptr_t a,uint64_t& v){auto it=words.find(a);if(it==words.end())return false;v=it->second;if(a==torn&&++reads>=2)++v;return true;}};
int main(){Reader r;uintptr_t base=0x100000;uint32_t self=0xa6053c,target=0x801e;uint32_t local=self&0x3fff;r.words[base+8*(local/64)]=1ULL<<(local%64);r.words[base]=1ULL<<30;
 auto v=npcvisibility::Read(r,base,target,self,0x3fff);assert(v.known&&v.visible&&v.index==30);
 r.words[base]=0;v=npcvisibility::Read(r,base,target,self,0x3fff);assert(v.known&&!v.visible); // zero TABLE bit != zero model mask
 r.words[base+8*(local/64)]=0;assert(!npcvisibility::Read(r,base,target,self,0x3fff).known);
 r.words[base+8*(local/64)]=1ULL<<(local%64);r.torn=base;r.reads=0;assert(!npcvisibility::Read(r,base,target,self,0x3fff).known);r.torn=0;
 assert(!npcvisibility::Read(r,base,target,self,0x7fff).known);assert(!npcvisibility::Read(r,base,0xffffffffu,self,0x3fff).known);
 r.words[base+8*255]=1ULL<<63;assert(npcvisibility::Read(r,base,0x7fff,self,0x3fff).visible);
 compacttop::Roster roster;roster.Begin(1,5,1);roster.Observe(2,0,1,"huskar");roster.Observe(2,1,2,"lion");roster.Observe(3,2,3,"lina");roster.Begin(2,5,1);assert(roster.Entries()[1].slot==1&&roster.Entries()[1].handle==2);roster.Observe(2,1,22,"lion");assert(roster.Entries()[1].handle==22);roster.Begin(3,6,1);assert(!roster.Entries()[0].handle);
 for(float W:{1120.f,1280.f,1920.f,2560.f}){float prior=-1;for(int team:{2,3})for(int slot=0;slot<5;++slot){auto b=compacttop::Place(W,1080,team,slot,76);assert(b.valid&&b.h==112&&b.capacity>=3&&b.capacity<=6&&b.x>=8&&b.x+b.w<=W-8&&b.y+b.h<1080);assert(b.x>prior);prior=b.x+b.w;assert(b.capacity*b.icon+(b.capacity-1)*2<=b.w-8);}}
 std::cout<<"PASS team NPC visibility bits: visible/hidden, self-bit validation, torn/missing/invalid handles, 16384-bit bounds; compact top slots/geometry at 1120/1280/1920/2560.\n";
}
