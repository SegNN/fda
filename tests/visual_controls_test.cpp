#include "../src/visual_controls.h"
#include <map>
#include <cassert>
#include <iostream>
struct Memory {std::map<uintptr_t,uint32_t> values;std::map<uintptr_t,uintptr_t> pointers;uintptr_t fail=0;int writes=0;bool Pointer(uintptr_t a,uintptr_t& n){if(!pointers.count(a))return false;n=pointers[a];return true;}bool Read(uintptr_t a,int bytes,uint32_t& n){if(!values.count(a))return false;n=values[a];return true;}bool Write(uintptr_t a,int bytes,uint32_t n){if(a==fail)return false;values[a]=n;++writes;return true;}};
int main(){using namespace visualcontrols;assert(!Parse("").valid&&!Parse("-1").valid&&!Parse("12G").valid&&!Parse("10,1,2,3,4").valid);assert(Parse("0x10,0x20").count==1);
 Memory m;uintptr_t base=0x100000;Path p[3]={Parse("10"),Parse("20"),Parse("30")};m.values[base+16]=FloatBits(1200);m.values[base+32]=FloatBits(-1);m.values[base+48]=1;uint32_t desired[3]={FloatBits(1600),FloatBits(3200),0};int sizes[3]={4,4,1};Group g;
 assert(!g.Tick(m,true,false,base,0x100,7,p,desired,sizes,3,false)&&m.writes==0);assert(g.Tick(m,true,true,base,0x100,7,p,desired,sizes,3,false));assert(Float(m.values[base+16])==1600&&m.values[base+48]==0);g.Tick(m,false,true,base,0x100,7,p,desired,sizes,3,false);assert(Float(m.values[base+16])==1200&&m.values[base+48]==1);
 m.fail=base+32;assert(!g.Tick(m,true,true,base,0x100,7,p,desired,sizes,3,false));assert(Float(m.values[base+16])==1200);m.fail=0;
 assert(g.Tick(m,true,true,base,0x100,7,p,desired,sizes,3,false));m.values[base+16]=FloatBits(1800);assert(!g.Tick(m,true,true,base,0x100,7,p,desired,sizes,3,false));g.Tick(m,false,true,base,0x100,7,p,desired,sizes,3,false);assert(Float(m.values[base+16])==1800);
 Path w[1]={Parse("40")};m.values[base+64]=0;uint32_t weather[1]={3};int width[1]={4};Group climate;assert(climate.Tick(m,true,true,base,0x100,7,w,weather,width,1,true)&&m.values[base+64]==3);climate.Tick(m,false,true,base,0x100,7,w,weather,width,1,true);assert(m.values[base+64]==0);
 m.pointers[base+0x50]=0x300000;assert(g.Resolve(m,base,0x100,Parse("50,20"))==0x300020);assert(!g.Resolve(m,base,0x100,Parse("FFFF")));
 std::cout<<"PASS ESP21 visual controls: no unconfirmed writes; bounded client pointer paths; camera/farz/fog types; weather 0..9 fixture; disable restore, foreign changes and write-failure rollback. No live ConVar validation.\n";
}
