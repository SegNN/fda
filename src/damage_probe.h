#pragma once
#include "buff_reader.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace damageprobe {
// Static dump candidates. Readability and stable identity do NOT prove live offsets/semantics.
// This reader is diagnostic only and MUST NOT feed Armlet actions.
enum class Kind {Float,Int,UInt};
struct Field {const char* cls;const char* name;uintptr_t offset;Kind kind;};
inline constexpr Field catalog[]={
#include "damage_probe_catalog.inc"
};
inline constexpr size_t CatalogSize=sizeof(catalog)/sizeof(catalog[0]);
struct Value {const Field* field=nullptr;bool read=false,stable=false,plausible=false;double raw=0;};
struct Snapshot {bool identity=false,knownClass=false;char cls[128]={};int count=0,totalFields=0;Value values[8]{};};
inline size_t LowerClass(const char* name){size_t lo=0,hi=CatalogSize;while(lo<hi){size_t mid=lo+(hi-lo)/2;if(strcmp(catalog[mid].cls,name)<0)lo=mid+1;else hi=mid;}return lo;}
template<class Reader> bool Identity(Reader& r,const buffreader::Buff& b,uint32_t owner,uintptr_t& vp){
 uint32_t parent=0;int serial=-1,index=-1;
 return owner&&b.parent==owner&&r.Valid(b.address)&&r.Owner(owner)&&r.Read(b.address,vp)&&r.Read(b.address+0x74,parent)&&parent==owner&&r.Read(b.address+0x48,serial)&&serial==b.serial&&r.Read(b.address+0x50,index)&&index==b.index;
}
template<class Reader> Snapshot Read(Reader& r,const buffreader::Buff& b,uint32_t owner){
 Snapshot out;uintptr_t vp=0;if(!Identity(r,b,owner,vp))return out;
 const char* cls=r.Class(vp);if(!cls||!cls[0]||strlen(cls)>=sizeof(out.cls))return out;
 snprintf(out.cls,sizeof(out.cls),"%s",cls);size_t at=LowerClass(cls);
 out.knownClass=at<CatalogSize&&!strcmp(catalog[at].cls,cls);
 for(size_t i=at;i<CatalogSize&&!strcmp(catalog[i].cls,cls);++i){++out.totalFields;if(out.count>=8)continue;
  auto& v=out.values[out.count++];v.field=&catalog[i];uint32_t a=0,c=0;
  v.read=r.Read(b.address+v.field->offset,a);v.stable=v.read&&r.Read(b.address+v.field->offset,c)&&a==c;
  if(!v.stable)continue;
  if(v.field->kind==Kind::Float){float f=0;memcpy(&f,&a,sizeof(f));v.raw=f;v.plausible=std::isfinite(f)&&fabsf(f)<=10000000.f;}
  else if(v.field->kind==Kind::Int){int32_t n=0;memcpy(&n,&a,sizeof(n));v.raw=n;v.plausible=n>=-10000000&&n<=10000000;}
  else {v.raw=a;v.plausible=a<=10000000u;}
 }
 uintptr_t again=0;if(!Identity(r,b,owner,again)||again!=vp)return Snapshot{};
 out.identity=true;return out;
}
inline int Format(char* text,size_t cap,const buffreader::Buff& b,const Snapshot& s){
 if(!text||!cap)return 0;
 int n=snprintf(text,cap,"probe modifier=%s class=%s identity=%d knownClass=%d candidateFields=%d shown=%d\n",b.name,s.cls[0]?s.cls:"unknown",s.identity,s.knownClass,s.totalFields,s.count);
 size_t used=n>0?std::min(size_t(n),cap-1):0;
 for(int i=0;i<s.count&&used<cap-1;++i){const auto& v=s.values[i];if(!v.field)continue;
  int added=0;
  if(v.read&&v.stable&&v.plausible)added=snprintf(text+used,cap-used,"  candidate %s +0x%llX raw=%g liveOffsetVerified=0\n",v.field->name,(unsigned long long)v.field->offset,v.raw);
  else added=snprintf(text+used,cap-used,"  candidate %s +0x%llX unknown read=%d stable=%d plausible=%d\n",v.field->name,(unsigned long long)v.field->offset,v.read,v.stable,v.plausible);
  if(added>0)used+=std::min(size_t(added),cap-used-1);
 }
 return int(used);
}
}
