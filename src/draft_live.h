#pragma once
#include <array>
#include <vector>
#include <cstdint>
struct Frame;
namespace draftadvisor {
struct Live {bool ok=false;int team=0,source=0,ownHero=-1;std::array<int,4> allies{{-1,-1,-1,-1}};std::vector<int> excluded;int playersLayout=-1,teamsLayout=-1;};
struct Probe {int stage=0,localPlayer=-1;uintptr_t resource=0,controller=0;uint64_t teamWord0=0,teamWord8=0,teamWord16=0,playersWord0=0,playersWord8=0,playersWord16=0;int readMask=0;};
inline Probe probe;
inline Live live;
inline bool automatic=true;
void Observe(const Frame& frame,double realtime);
}
