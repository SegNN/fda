#pragma once
#include "game.h"
namespace localvisibility {
struct State {bool known=false,visible=false;int source=0;};
// source 1=team NPC bit table candidate, 2=nonzero legacy model mask, 3=ally.
inline State Get(const Frame& f,const FrameUnit& u){
 if(!f.ok||f.observedOnly||(f.localTeam!=2&&f.localTeam!=3))return {};
 if(u.team==f.localTeam)return {true,true,3};if(u.team!=2&&u.team!=3)return {};
 if(u.npcVisibilityRead)return {true,u.npcVisible,1};
 if(!cfg::vbe&&u.teamVisibilityRead&&u.teamVisibilityMask)return {true,(u.teamVisibilityMask&(1u<<f.localTeam))!=0,2};
 return {}; // Readable zero model mask is NOT proof that a rendered NPC is in fog.
}
}
