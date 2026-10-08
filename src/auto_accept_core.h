#pragma once
#include "cosmetic_core.h"
#include <cmath>
#include <set>
#include <deque>
namespace acceptcore {
struct Image {int width=0,height=0;std::vector<uint8_t> rgb;};
inline bool Informative(const Image& im){
 if(im.width!=160||im.height!=40||im.rgb.size()!=19200)return false;
 double sum=0,sq=0;int edges=0;
 auto grey=[&](int i){return (im.rgb[i*3]+im.rgb[i*3+1]+im.rgb[i*3+2])/3.;};
 for(int i=0;i<6400;++i){double g=grey(i);sum+=g;sq+=g*g;if(i%160&&std::abs(g-grey(i-1))>20)++edges;}
 double mean=sum/6400.;return sq/6400.-mean*mean>100&&edges>=80;
}
inline bool Matches(const Image& a,const Image& b){
 if(!Informative(a)||!Informative(b))return false;
 double delta=0;size_t close=0;
 for(size_t i=0;i<a.rgb.size();++i){int d=std::abs(int(a.rgb[i])-int(b.rgb[i]));delta+=d;if(d<=24)++close;}
 return delta/a.rgb.size()<=6 && close>=a.rgb.size()*.98;
}
class Gate {
 std::set<uint64_t> attempted;std::deque<uint64_t> history;
 uint64_t lobby=0,received=0,epoch=0;bool confirmed=false;bool enabled=false;int stable=0;uint64_t lastSample=0;
public:
 void Enable(bool on,uint64_t now){if(on==enabled)return;enabled=on;epoch=now;lobby=0;received=0;confirmed=false;stable=0;lastSample=0;}
 void Observe(uint32_t type,const cosmetic::Bytes& raw,uint64_t now){
  if((type&~cosmetic::ProtoFlag)!=7170)return;
  try {
   auto env=cosmetic::Unpack(type,raw);auto fields=cosmetic::pb::Decode(env.body);
   uint64_t id=0;int state=0,ids=0,states=0; // protobuf schema default: UNDECLARED
   for(const auto& f:fields){if(f.n==1){++ids;id=f.wire==1?f.value:0;}if(f.n==6){++states;state=f.wire==0&&f.value<=3?(int)f.value:-1;}}
   if(ids==1&&states==1&&id&&state==1&&attempted.count(id))confirmed=true;
   if(ids!=1||states>1||!id||state!=0){lobby=0;stable=0;return;}
   if(!enabled||now<=epoch)return;
   if(lobby!=id){confirmed=false;stable=0;lastSample=0;}lobby=id;received=now;
  }catch(...){lobby=0;stable=0;}
 }
 bool Confirmed()const{return confirmed;}
 bool Fresh(uint64_t now)const{return enabled&&lobby&&!attempted.count(lobby)&&now>=received&&now-received<=8000;}
 bool Sample(uint64_t now,bool visual,bool foreground,bool menuClosed,bool noMatch,bool templateReady){
  if(!Fresh(now)||!foreground||!menuClosed||!noMatch||!templateReady||!visual){stable=0;return false;}
  if(lastSample&&now-lastSample<200)return false;
  if(lastSample&&now-lastSample>1000)stable=0;
  lastSample=now;return ++stable>=3;
 }
 void Attempt(){attempted.insert(lobby);history.push_back(lobby);while(history.size()>64){attempted.erase(history.front());history.pop_front();}stable=0;}
};
}
