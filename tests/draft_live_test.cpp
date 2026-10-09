#include "draft_live.cpp"
#include <cassert>
#include <unordered_map>
#include <iostream>
namespace game {Sys g_sys;std::atomic<uintptr_t> g_playerResource{0};std::unordered_map<uint32_t,uintptr_t> handles;uintptr_t EntityByHandle(uint32_t h){auto i=handles.find(h);return i==handles.end()?0:i->second;}}
void Object(uintptr_t a,uintptr_t vp,uint32_t handle,const char* cls){mem::Put(a,vp);rtti::classes[vp]=cls;mem::Put(a+off::instEntity,a+0x4000);mem::Put(a+0x4000+off::idHandleFld,handle);game::handles[handle]=a;}
int main(){using namespace draftadvisor;uintptr_t ctrl=0x100100000ULL,resource=0x100200000ULL,players=0x100300000ULL,teams=0x100400000ULL;
 game::g_sys.ready=true;game::g_sys.clientBase=0x100000000ULL;game::g_sys.localCtrl=ctrl;game::g_playerResource=resource;
 Object(ctrl,0x100010000ULL,1,"C_DOTAPlayerController");Object(resource,0x100020000ULL,2,"C_DOTA_PlayerResource");
 mem::Put(ctrl+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));mem::Put(ctrl+off::Ctrl::m_nPlayerID,0);
 mem::Put(resource+0x670,10);mem::Put(resource+0x678,players);mem::Put(resource+0x608,10);mem::Put(resource+0x610,teams);
 for(int i=0;i<10;++i){mem::Put(players+i*0xf0+0x30,uint8_t(1));mem::Put(players+i*0xf0+0x40,i<5?2:3);mem::Put(teams+i*0x238+0x98,i+1);}
 Frame none;Observe(none,100);assert(!live.ok);Observe(none,100.3);assert(live.ok&&live.source==2&&live.team==2&&live.ownHero==IndexById(1)&&live.allies[0]==IndexById(2)&&live.excluded.size()==10);
 mem::Put(teams+0x238+0x98,11);Observe(none,100.6);assert(!live.ok);Observe(none,100.9);assert(live.ok&&live.allies[0]==IndexById(11));
 mem::Put(ctrl+off::Ctrl::m_bIsLocalPlayerController,uint8_t(0));Observe(none,101.2);assert(!live.ok);
 mem::Put(ctrl+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));Observe(none,101.5);assert(!live.ok);Observe(none,101.8);assert(live.ok);
 Object(ctrl,0x100010000ULL,0x4001,"C_DOTAPlayerController");Observe(none,102.1);assert(!live.ok);Observe(none,102.4);assert(live.ok);
 game::g_playerResource=0;Observe(none,102.7);assert(!live.ok);
 Frame match;match.ok=true;match.localTeam=2;match.localHero=0x100500000ULL;match.localHandle=0x10001;
 FrameUnit self;self.kind=UnitKind::Hero;self.addr=match.localHero;self.entityHandle=match.localHandle;self.team=2;self.playerId=0;strcpy(self.name,"npc_dota_hero_huskar");FrameUnit ally=self;ally.addr+=0x1000;ally.entityHandle+=1;ally.playerId=1;strcpy(ally.name,"npc_dota_hero_crystal_maiden");match.units={self,ally,ally};Observe(match,103);assert(live.ok&&live.source==1&&live.ownHero==IndexByName(self.name)&&live.allies[0]==IndexByName(ally.name)&&live.allies[1]==-1);
 match.observedOnly=true;Observe(match,103.3);assert(!live.ok);automatic=false;Observe(match,103.6);assert(!live.ok);
 std::cout<<"PASS production draft observer with mock memory: pre-spawn two-read roster, identity/serial/local flag, changed draft, unavailable resource, current-match fallback, no duplicate teammate, observed-only and manual mode. Not live schema validation.\n";
}
