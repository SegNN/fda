#include "../src/damage_probe.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <string>
#include <limits>
struct Reader {
 std::unordered_map<uintptr_t,uint8_t> mem;std::string cls="CDOTA_Modifier_Viper_PoisonAttack_Slow";uint32_t owner=0x17B0505;uintptr_t changeField=0,changeParent=0;int fieldReads=0,parentReads=0;
 bool Valid(uintptr_t a){return a>=0x100000000ULL;}
 bool Owner(uint32_t h){return h==owner;}
 const char* Class(uintptr_t){return cls.c_str();}
 template<class T>void Put(uintptr_t a,T v){auto p=reinterpret_cast<uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i)mem[a+i]=p[i];}
 template<class T>bool Read(uintptr_t a,T& v){
  if(a==changeField&&++fieldReads==2)Put(a,uint32_t(0x42C80000));
  if(a==changeParent&&++parentReads==2)Put(a,uint32_t(owner+0x4000));
  auto p=reinterpret_cast<uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i){auto it=mem.find(a+i);if(it==mem.end())return false;p[i]=it->second;}return true;
 }
};
int main(){
 Reader r;buffreader::Buff b;b.address=0x400000000ULL;b.parent=r.owner;b.serial=4;b.index=2;strcpy(b.name,"modifier_viper_poison_attack_slow");
 r.Put(b.address,uintptr_t(0x700000000ULL));r.Put(b.address+0x74,b.parent);r.Put(b.address+0x48,b.serial);r.Put(b.address+0x50,b.index);
 r.Put(b.address+0x1AA8,25.f);r.Put(b.address+0x1ABC,50.f);r.Put(b.address+0x1AC4,int32_t(20));
 auto s=damageprobe::Read(r,b,b.parent);assert(s.identity&&s.knownClass&&s.totalFields>0);
 bool damage=false;for(int i=0;i<s.count;++i)if(!strcmp(s.values[i].field->name,"damage")){assert(s.values[i].read&&s.values[i].stable&&s.values[i].plausible&&s.values[i].raw==25);damage=true;}assert(damage);
 char text[4096];damageprobe::Format(text,sizeof(text),b,s);assert(strstr(text,"liveOffsetVerified=0"));assert(strstr(text,"unknown read=0"));
 r.Put(b.address+0x1AA8,std::numeric_limits<float>::quiet_NaN());s=damageprobe::Read(r,b,b.parent);for(int i=0;i<s.count;++i)if(!strcmp(s.values[i].field->name,"damage"))assert(!s.values[i].plausible);
 r.Put(b.address+0x1AA8,25.f);r.changeField=b.address+0x1AA8;s=damageprobe::Read(r,b,b.parent);for(int i=0;i<s.count;++i)if(!strcmp(s.values[i].field->name,"damage"))assert(!s.values[i].stable);
 r.changeField=0;r.Put(b.address+0x48,5);assert(!damageprobe::Read(r,b,b.parent).identity);r.Put(b.address+0x48,4);
 assert(!damageprobe::Read(r,b,b.parent+0x4000).identity);
 r.cls="CDOTA_Modifier_Fictional_Unknown";s=damageprobe::Read(r,b,b.parent);assert(s.identity&&!s.knownClass&&s.count==0);
 r.cls="C_DOTA_BaseNPC_Hero";s=damageprobe::Read(r,b,b.parent);assert(!s.knownClass&&s.count==0);
 r.cls="CDOTA_Modifier_Armlet_UnholyStrength";r.Put(b.address+0x1AB8,int32_t(45));r.Put(b.address+0x1AC4,.1f);s=damageprobe::Read(r,b,b.parent);assert(s.identity&&s.knownClass);
 r.changeParent=b.address+0x74;assert(!damageprobe::Read(r,b,b.parent).identity);
 char small[12];damageprobe::Format(small,sizeof(small),b,s);assert(small[11]=='\0');
 for(size_t i=1;i<damageprobe::CatalogSize;++i)assert(strcmp(damageprobe::catalog[i-1].cls,damageprobe::catalog[i].cls)<=0);
 std::cout<<"PASS read-only damage probe: 915 class candidates, Viper/Armlet raw fields, stale ownership/serial, identity mutation, unstable fields, NaN, missing/unknown fields, bounded report; NOT live-offset or damage-model validation\n";
}
