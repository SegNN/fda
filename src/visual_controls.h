#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <array>
#include <algorithm>
namespace visualcontrols {
struct Path {uint64_t rva=0;std::array<uint64_t,3> offsets{};int count=0;bool valid=false;};
inline Path Parse(const char* text){Path p;if(!text||!*text)return p;const char* c=text;uint64_t values[4]{};int n=0;
 while(*c){while(*c==' '||*c==',')++c;if(!*c)break;if(n==4||*c=='-'||*c=='+')return {};
  char* end=nullptr;auto value=std::strtoull(c,&end,16);if(end==c||(*end&&*end!=','&&*end!=' ')||value>0x7fffffffULL)return {};values[n++]=value;c=end;}
 if(!n||!values[0])return p;p.rva=values[0];p.count=n-1;for(int i=1;i<n;++i)p.offsets[i-1]=values[i];p.valid=true;return p;
}
struct Value {uintptr_t address=0;uint32_t original=0,last=0;int bytes=4;};
inline uint32_t FloatBits(float f){uint32_t n;std::memcpy(&n,&f,4);return n;}
inline float Float(uint32_t n){float f;std::memcpy(&f,&n,4);return f;}
class Group {
 std::array<Value,3> values{};bool applied=false;uintptr_t context=0;int count=0;
public:
 const char* status="Disabled / verified client pointer paths required";
 template<class Memory> uintptr_t Resolve(Memory& m,uintptr_t base,uint64_t image,const Path& p){
  if(!base||!p.valid||image<8||p.rva>image-8)return 0;uintptr_t at=base+p.rva;
  for(int i=0;i<p.count;++i){uintptr_t ptr=0,again=0;if(!m.Pointer(at,ptr)||!m.Pointer(at,again)||ptr!=again||ptr<0x10000||ptr>0x00007fffffffffffULL||ptr+p.offsets[i]<ptr)return 0;at=ptr+p.offsets[i];}return at;
 }
 template<class Memory> bool Restore(Memory& m){bool ok=true;for(int i=0;i<count;++i){uint32_t current=0;if(!m.Read(values[i].address,values[i].bytes,current)||current!=values[i].last){ok=false;continue;}if(!m.Write(values[i].address,values[i].bytes,values[i].original))ok=false;}applied=false;count=0;return ok;}
 template<class Memory> bool Tick(Memory& m,bool enabled,bool authorized,uintptr_t base,uint64_t image,uintptr_t owner,const Path* paths,const uint32_t* desired,const int* widths,int n,bool weather){
  if(n<1||n>3)return false;
  if(applied){bool same=context==owner;for(int i=0;i<count;++i)same=same&&Resolve(m,base,image,paths[i])==values[i].address;
   if(!same){applied=false;count=0;status="Context/path changed; no stale restoration write";return false;}}
  if(!enabled||!authorized){if(applied)status=Restore(m)?"Original values restored":"Restore skipped/failed: value no longer owned";else status=authorized?"Disabled":"Blocked: confirm private demo and valid local context";return false;}
  std::array<Value,3> next{};for(int i=0;i<n;++i){auto at=Resolve(m,base,image,paths[i]);uint32_t original=0,again=0;if(!at||!m.Read(at,widths[i],original)||!m.Read(at,widths[i],again)||again!=original){status="Blocked: pointer/value unavailable or unstable";return false;}
   bool plausible=weather?original<=9:i==0?(std::isfinite(Float(original))&&Float(original)>=100&&Float(original)<=6000):i==1?(std::isfinite(Float(original))&&(Float(original)==-1||(Float(original)>=0&&Float(original)<=1000000))):original<=1;
   if(!plausible){status="Blocked: value type/range not plausible";return false;}
   if(applied&&original!=values[i].last){status="Blocked: game/user changed the value; no overwrite";return false;}
   for(int j=0;j<i;++j)if(at<next[j].address+next[j].bytes&&next[j].address<at+widths[i]){status="Blocked: overlapping value addresses";return false;}
   next[i]={at,applied?values[i].original:original,desired[i],widths[i]};}
  for(int i=0;i<n;++i){if(!m.Write(next[i].address,next[i].bytes,next[i].last)){for(int j=0;j<i;++j)m.Write(next[j].address,next[j].bytes,applied?values[j].last:next[j].original);status="Write failed; rollback of preceding writes attempted";return false;}}
  values=next;count=n;context=owner;applied=true;status="Client values applied; LIVE feature correctness unverified";return true;
 }
};
}
