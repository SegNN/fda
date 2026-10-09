#include "game.h"
#include "draft_live.h"
#include "draft_lane_planner.h"
#include "draft_reader_core.h"
namespace draftadvisor {
struct Reader {template<class T>bool Read(uintptr_t a,T& v){return mem::Read(a,v);}};
static bool Identity(uintptr_t object,const char* expected,uint32_t& handle){uintptr_t vp=0,id=0;
 return mem::ValidPtr(object)&&mem::Read(object,vp)&&Streq(rtti::ClassOf(game::g_sys.clientBase,vp),expected)&&
 mem::Read(object+off::instEntity,id)&&mem::Read(id+off::idHandleFld,handle)&&game::EntityByHandle(handle)==object;
}
void Observe(const Frame& f,double now){
 static double next=0;static draftreader::Result pending;static uintptr_t priorResource=0;static uint32_t priorControllerHandle=0,priorResourceHandle=0;
 if(!std::isfinite(now)){live={};return;}if(now<next-.3){next=now;pending={};priorResource=0;}if(now<next)return;next=now+.25;live={};probe={};
 if(!automatic){pending={};return;}Reader r;uintptr_t ctrl=game::g_sys.localCtrl,resource=game::g_playerResource.load();
 probe.resource=resource;probe.controller=ctrl;probe.stage=1;
 if(resource){if(mem::Read(resource+0x608,probe.teamWord0))probe.readMask|=1;if(mem::Read(resource+0x610,probe.teamWord8))probe.readMask|=2;if(mem::Read(resource+0x618,probe.teamWord16))probe.readMask|=4;
 if(mem::Read(resource+0x670,probe.playersWord0))probe.readMask|=8;if(mem::Read(resource+0x678,probe.playersWord8))probe.readMask|=16;if(mem::Read(resource+0x680,probe.playersWord16))probe.readMask|=32;}
 uint32_t ch=0,rh=0,ch2=0,rh2=0;int local=-1,local2=-1;uint8_t flag=0,flag2=0;
 if(game::g_sys.ready&&Identity(ctrl,"C_DOTAPlayerController",ch)&&Identity(resource,"C_DOTA_PlayerResource",rh)&&
  draftreader::Stable(r,ctrl+off::Ctrl::m_bIsLocalPlayerController,flag)&&flag==1&&draftreader::Stable(r,ctrl+off::Ctrl::m_nPlayerID,local)){
  probe.stage=2;probe.localPlayer=local;
  auto candidate=draftreader::Read(r,resource,local,[](int id){return IndexById(id)>=0;});
  bool consistent=candidate.ok&&Identity(ctrl,"C_DOTAPlayerController",ch2)&&ch2==ch&&Identity(resource,"C_DOTA_PlayerResource",rh2)&&rh2==rh&&
    mem::Read(ctrl+off::Ctrl::m_nPlayerID,local2)&&local2==local&&mem::Read(ctrl+off::Ctrl::m_bIsLocalPlayerController,flag2)&&flag2==flag;
  if(consistent){probe.stage=3;bool stable=priorResource==resource&&priorControllerHandle==ch&&priorResourceHandle==rh&&draftreader::Equal(pending,candidate);pending=candidate;priorResource=resource;priorControllerHandle=ch;priorResourceHandle=rh;
   if(stable){probe.stage=4;live.ok=true;live.team=candidate.team;live.source=2;live.ownHero=IndexById(candidate.ownHero);live.playersLayout=candidate.layoutPlayers;live.teamsLayout=candidate.layoutTeams;
    for(int i=0;i<4;++i)live.allies[i]=IndexById(candidate.allies[i]);for(int id:candidate.taken)live.excluded.push_back(IndexById(id));return;}
   return; // Changed draft waits for another independent read, not stale roster.
  }
 }
 pending={};priorResource=0;
 // Read-only match fallback. This cannot discover pre-spawn draft picks.
 if(!f.ok||f.observedOnly||(f.localTeam!=2&&f.localTeam!=3))return;
 std::array<int,64> byPlayer;byPlayer.fill(-1);bool conflicting=false;
 for(const auto& u:f.units){if(u.kind!=UnitKind::Hero||u.illusion||!u.entityHandle||u.playerId<0||u.playerId>=64)continue;int hero=IndexByName(u.name);if(hero<0)continue;
  if(byPlayer[u.playerId]>=0&&byPlayer[u.playerId]!=hero){conflicting=true;break;}byPlayer[u.playerId]=hero;
 }
 if(conflicting)return;int n=0;std::vector<int> seen;
 for(const auto& u:f.units){if(u.kind!=UnitKind::Hero||u.illusion||u.playerId<0||u.playerId>=64||!u.entityHandle)continue;int hero=IndexByName(u.name);if(hero<0||std::find(seen.begin(),seen.end(),u.playerId)!=seen.end())continue;
  seen.push_back(u.playerId);live.excluded.push_back(hero);if(u.addr==f.localHero&&u.entityHandle==f.localHandle)live.ownHero=hero;if(u.team==f.localTeam&&u.addr!=f.localHero&&u.entityHandle!=f.localHandle){if(n>=4){live={};return;}live.allies[n++]=hero;}}
 probe.stage=5;live.ok=true;live.team=f.localTeam;live.source=1;
}
}
