#pragma once
#include <array>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace compacttop {
struct Entry {int team=0,player=-1,slot=-1;uint32_t handle=0;std::string name;};
class Roster {
 std::array<Entry,10> entries{};double last=-1;uintptr_t rules=0;uint32_t local=0;
public:
 void Reset(){entries={};last=-1;rules=0;local=0;}
 void Begin(double now,uintptr_t match,uint32_t owner){if(!std::isfinite(now)){Reset();return;}if(last>=0&&(now<last||match!=rules||local!=owner))Reset();last=now;rules=match;local=owner;}
 void Observe(int team,int player,uint32_t handle,const char* name){if((team!=2&&team!=3)||!handle||handle==0xffffffffu||!name)return;
  for(auto& e:entries)if(e.team==team&&((player>=0&&e.player==player)||(player<0&&e.handle==handle))){e.handle=handle;e.name=name;return;}
  int base=(team-2)*5;for(int i=0;i<5;++i)if(!entries[base+i].handle){entries[base+i]={team,player,i,handle,name};return;}
 }
 const std::array<Entry,10>& Entries()const{return entries;}
};
struct Box {float x=0,y=0,w=0,h=68,icon=24;int capacity=6;bool valid=false;};
inline Box Place(float W,float H,int team,int slot,float y){Box b;if(!std::isfinite(W)||!std::isfinite(H)||!std::isfinite(y)||W<900||H<300||(team!=2&&team!=3)||slot<0||slot>=5)return b;
 float centerGap=std::clamp(W*.075f,110.f,160.f),gap=4;
 b.w=std::min(168.f,(W-32-centerGap)/10-gap);b.icon=b.w>=154?24.f:18.f;b.capacity=std::clamp(int((b.w-8+2)/(b.icon+2)),1,6);
 float span=5*(b.w+gap)-gap;b.x=(team==2?(W-centerGap)*.5f-span:(W+centerGap)*.5f)+slot*(b.w+gap);
 b.y=std::clamp(y,54.f,H-b.h-8);b.valid=b.x>=8&&b.x+b.w<=W-8;return b;
}
}
