#include "../src/buff_reader.h"
#include <unordered_map>
#include <cassert>
#include <iostream>
struct Memory {
 std::unordered_map<uintptr_t,uint8_t> m;bool owner=true;
 template<class T>void Put(uintptr_t a,T v){for(size_t i=0;i<sizeof(T);++i)m[a+i]=((uint8_t*)&v)[i];}
 template<class T>bool Read(uintptr_t a,T& v){for(size_t i=0;i<sizeof(T);++i)if(!m.count(a+i))return false;for(size_t i=0;i<sizeof(T);++i)((uint8_t*)&v)[i]=m[a+i];return true;}
 bool Valid(uintptr_t p){return p>=0x100000000ULL&&(p&7)==0;}
 bool BuffClass(uintptr_t p){return p==0x900000000ULL;}
 bool Owner(uint32_t p){return owner&&p==0x8001;}
 bool String(uintptr_t a,char* out,int n){for(int i=0;i<n;++i){if(!Read(a+i,out[i]))return false;if(!out[i])return true;}return false;}
 void Buff(uintptr_t p,uintptr_t str,int index,const char* name){Put(p,uintptr_t(0x900000000ULL));Put(p+0x28,str);for(size_t i=0;i<=strlen(name);++i)Put(str+i,name[i]);Put(p+0x74,uint32_t(0x8001));Put(p+0x48,index+10);Put(p+0x50,index);Put(p+0x54,100.f);Put(p+0x60,10.f);Put(p+0x64,110.f);Put(p+0x84,1);}
};
int main(){Memory r;uintptr_t mgr=0x100000000ULL,data=0x200000000ULL,b=0x300000000ULL;
 for(int i=0;i<40;++i)r.Put(mgr+i,uint8_t(0));r.Put(mgr+16,1);r.Put(mgr+24,data);r.Put(data,b);r.Buff(b,0x400000000ULL,1,"modifier_flask_healing");
 buffreader::Layout layout;auto out=buffreader::Read(r,mgr,0x8001,105,layout);assert(out.verified&&out.buffs.size()==1);assert(buffreader::Remaining(out.buffs[0],105)==5);
 r.owner=false;assert(!buffreader::Read(r,mgr,0x8001,105,layout).verified);r.owner=true;
 r.Put(b+0x74,uint32_t(0xC001));assert(!buffreader::Read(r,mgr,0x8001,105,layout).verified);r.Put(b+0x74,uint32_t(0x8001));
 r.Put(mgr+16,0);layout={};assert(!buffreader::Read(r,mgr,0x8001,105,layout).verified); // unknown empty isn't verified
 r.Put(mgr+16,1);out=buffreader::Read(r,mgr,0x8001,105,layout);assert(out.verified);
 r.Put(mgr+16,0);assert(buffreader::Read(r,mgr,0x8001,105,layout).verified); // previously verified layout, empty
 r.Put(mgr+16,1);r.Put(mgr+20,1);layout={};out=buffreader::Read(r,mgr,0x8001,105,layout);assert(!out.verified&&out.visualOnly&&out.buffs.size()==1&&layout.count<0); // identical positive snapshot, automation still BLOCKED
 r.Put(mgr+20,0);r.Put(b+0x60,-1.f);out=buffreader::Read(r,mgr,0x8001,105,layout);assert(out.verified&&buffreader::Remaining(out.buffs[0],105)==-1);
 r.Put(b+0x60,10.f);r.Put(b+0x64,0.f);out=buffreader::Read(r,mgr,0x8001,105,layout);assert(out.verified&&buffreader::Remaining(out.buffs[0],105)==-2);
 std::cout<<"PASS buff ownership, serials, bounded discovery, ambiguous layouts, unknown empty and timer semantics\n";
}
