#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
// Read-only schema fields. Private vector layout is established by validating every entry,
// not asserted from an old cheat offset. An empty unknown vector is never "no buffs".
namespace buffreader {
struct Buff { uintptr_t address=0; uint32_t parent=0,ability=0; int serial=0,index=0,stacks=0; float created=0,duration=-1,expires=-1; char name[128]={}; };
struct Result { bool verified=false; std::vector<Buff> buffs; };
struct Layout { int count=-1,data=-1; };
inline bool Time(float t){return std::isfinite(t)&&t>=-120&&t<=86400;}
template<class Reader> bool Entry(Reader& r,uintptr_t addr,uint32_t owner,float now,Buff& b){
 if(!r.Valid(addr))return false;uintptr_t vp=0,name=0;uint32_t parent=0;
 if(!r.Read(addr,vp)||!r.BuffClass(vp)||!r.Read(addr+0x74,parent)||parent!=owner||!r.Owner(parent))return false;
 b=Buff{};b.address=addr;b.parent=parent;
 if(!r.Read(addr+0x28,name)||!r.String(name,b.name,128)||strncmp(b.name,"modifier_",9)!=0)return false;
 for(const char* c=b.name;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||*c=='_'))return false;
 if(!r.Read(addr+0x48,b.serial)||!r.Read(addr+0x50,b.index)||!r.Read(addr+0x54,b.created)||
    !r.Read(addr+0x60,b.duration)||!r.Read(addr+0x64,b.expires)||!r.Read(addr+0x84,b.stacks))return false;
 r.Read(addr+0x70,b.ability);
 if(b.serial<0||b.index<0||b.index>65535||b.stacks<0||b.stacks>100000||!Time(b.created)||b.created>now+1||
    !std::isfinite(b.duration)||b.duration < -1||b.duration>86400||!Time(b.expires))return false;
 uint32_t parent2=0;int serial2=0;
 return r.Read(addr+0x74,parent2)&&r.Read(addr+0x48,serial2)&&parent2==parent&&serial2==b.serial;
}
template<class Reader> Result Candidate(Reader& r,uintptr_t mgr,Layout l,uint32_t owner,float now,bool allowEmpty){
 Result out;int count=0;uintptr_t data=0;
 if(!r.Read(mgr+l.count,count)||!r.Read(mgr+l.data,data)||count<0||count>256)return out;
 if(count==0){int again=-1;uintptr_t againData=0;out.verified=allowEmpty&&r.Read(mgr+l.count,again)&&r.Read(mgr+l.data,againData)&&again==0&&againData==data;return out;}
 if(!r.Valid(data))return out;
 for(int i=0;i<count;++i){uintptr_t ptr=0;Buff b;if(!r.Read(data+8ULL*i,ptr)||!Entry(r,ptr,owner,now,b))return {};
  if(std::any_of(out.buffs.begin(),out.buffs.end(),[&](const Buff& x){return x.address==b.address||x.index==b.index;}))return {};
  out.buffs.push_back(b);
 }
 int count2=-1;uintptr_t data2=0;
 if(!r.Read(mgr+l.count,count2)||!r.Read(mgr+l.data,data2)||count2!=count||data2!=data)return {};
 out.verified=true;return out;
}
template<class Reader> Result Read(Reader& r,uintptr_t mgr,uint32_t owner,float now,Layout& learned){
 if(!owner||!std::isfinite(now))return {};
 if(learned.count>=0){auto out=Candidate(r,mgr,learned,owner,now,true);if(out.verified)return out;learned={};}
 // Only the manager's opaque 0x00..0x27 prefix. No heap scan, no virtual calls.
 // Pointer/count pair layouts of CUtlVector and network-vector containers are hypotheses,
 // accepted only when an entire non-empty array has independently validated buffs.
 Result found;Layout chosen;int matches=0;
 for(int c=0;c<=0x20;c+=4)for(int d=0;d<=0x20;d+=8){
  if((c<d+8&&c+4>d))continue;
  Layout l{c,d};auto out=Candidate(r,mgr,l,owner,now,false);if(!out.verified)continue;
  if(matches)return {}; // Ambiguous count/capacity fields are not a proven layout.
  found=std::move(out);chosen=l;++matches;
 }
 if(matches){learned=chosen;return found;}return {};
}
inline float Remaining(const Buff& b,float now){
 if(b.duration<0)return -1;
 if(!std::isfinite(now)||b.expires<=0||b.expires<b.created)return -2; // Unknown, never guessed.
 return std::max(0.f,b.expires-now);
}
}
