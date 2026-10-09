#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <cctype>
#include <cstring>
// Display-only observed-vision memory. Full handles, no action or live hidden coordinates.
namespace fogmemory {
struct Position {float x=0,y=0,z=0;};
inline std::string PortraitKey(const char* name){
 if(!name)return {};std::string key;const char* prefix="npc_dota_hero_";
 if(std::strncmp(name,prefix,14)==0)name+=14;
 for(int i=0;name[i];++i){if(i>=79)return {};unsigned char c=(unsigned char)name[i];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return {};key+=(char)std::tolower(c);}
 return key;
}
struct Marker {uint32_t handle=0;Position pos;float remaining=0;std::string portrait;};
class Tracker {
 struct State {Position pos;double lost=-1;bool visible=false;std::string portrait;};
 std::unordered_map<uint32_t,State> states;
 std::unordered_set<uint32_t> touched;
 double now=0,last=-1;
public:
 void Reset(){states.clear();touched.clear();last=-1;}
 void Begin(double t){if(!std::isfinite(t)){Reset();now=0;return;}if(last>=0&&t<last)Reset();now=t;last=t;touched.clear();}
 void Observe(uint32_t h,bool alive,bool known,bool visible,Position pos,const char* portrait=""){
  if(!h||h==0xffffffffu)return;touched.insert(h);
  if(!alive){states.erase(h);return;}
  auto it=states.find(h);
  if(!known){if(it!=states.end())states.erase(it);return;} // Unknown is not fog.
  if(visible){if(!std::isfinite(pos.x)||!std::isfinite(pos.y)||!std::isfinite(pos.z)){states.erase(h);return;}
   if(states.size()>=256&&it==states.end())return;states[h]={pos,-1,true,PortraitKey(portrait)};
  }else if(it!=states.end()&&it->second.visible){it->second.visible=false;it->second.lost=now;}
 }
 std::vector<Marker> End(){std::vector<Marker> out;
  for(auto it=states.begin();it!=states.end();){auto& v=it->second;
   if(!touched.count(it->first)&&v.visible){v.visible=false;v.lost=now;}
   if(!v.visible){double left=10.-(now-v.lost);if(left<=0){it=states.erase(it);continue;}out.push_back({it->first,v.pos,float(std::min(10.,left)),v.portrait});}
   ++it;
  }return out;
 }
};
}
