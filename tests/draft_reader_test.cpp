#include "draft_reader_core.h"
#include <unordered_map>
#include <cstring>
#include <cassert>
#include <iostream>
struct Memory{std::unordered_map<uintptr_t,unsigned char> bytes;uintptr_t torn=0;int hits=0;
 template<class T>void Put(uintptr_t a,T v){for(size_t i=0;i<sizeof(v);++i)bytes[a+i]=((unsigned char*)&v)[i];}
 template<class T>bool Read(uintptr_t a,T& v){for(size_t i=0;i<sizeof(v);++i)if(!bytes.count(a+i))return false;for(size_t i=0;i<sizeof(v);++i)((unsigned char*)&v)[i]=bytes[a+i];if(a==torn&&++hits>1)v=T(99);return true;}
};
int main(){Memory r;uintptr_t resource=0x100001000ULL,p=0x100002000ULL,t=0x100006000ULL;
 r.Put(resource+0x670,10);r.Put(resource+0x678,p);r.Put(resource+0x608,10);r.Put(resource+0x610,t);
 for(int i=0;i<10;++i){r.Put(p+i*0xf0+0x30,uint8_t(1));r.Put(p+i*0xf0+0x40,i<5?2:3);r.Put(t+i*0x238+0x98,i+1);}
 auto known=[](int id){return id>=1&&id<=155;};auto v=draftreader::Read(r,resource,0,known);assert(v.ok&&v.team==2&&v.allies[0]==2&&v.allies[3]==5&&v.taken.size()==10);
 assert(!draftreader::Read(r,resource,-1,known).ok);r.Put(t+0x238+0x98,999);assert(!draftreader::Read(r,resource,0,known).ok);r.Put(t+0x238+0x98,2);
 r.torn=p+0x40;assert(!draftreader::Read(r,resource,0,known).ok);r.torn=0;r.hits=0;
 r.Put(t+0x238+0x98,1);assert(!draftreader::Read(r,resource,0,known).ok);r.Put(t+0x238+0x98,0);assert(draftreader::Read(r,resource,0,known).ok); // Not picked yet.
 r.Put(p+0x30,uint8_t(0));assert(!draftreader::Read(r,resource,0,known).ok);r.Put(p+0x30,uint8_t(1));r.Put(resource+0x670,11);r.Put(resource+0x608,11);r.Put(p+10*0xf0+0x30,uint8_t(1));r.Put(p+10*0xf0+0x40,1);assert(draftreader::Read(r,resource,0,known).ok);assert(!draftreader::Read(r,resource,10,known).ok);r.Put(resource+0x670,65);assert(!draftreader::Read(r,resource,0,known).ok);
 std::cout<<"PASS draft candidate reader: bounded count, stable fields, local player, teams, unknown/duplicate IDs, unpicked IDs, torn reads and malformed headers. Static schema candidates, not live offsets.\n";
}
