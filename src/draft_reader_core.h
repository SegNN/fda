#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <algorithm>
namespace draftreader {
struct Result {bool ok=false;int team=0,local=-1,ownHero=0;std::array<int,4> allies{{0,0,0,0}};std::vector<int> taken;int layoutPlayers=-1,layoutTeams=-1;};
struct Header{int count=0;uintptr_t data=0;};
inline bool Ptr(uintptr_t a){return a>=0x100000000ULL&&a<0x7fffffffffffULL&&(a&7)==0;}
inline bool Equal(const Result&a,const Result&b){return a.ok==b.ok&&a.team==b.team&&a.local==b.local&&a.ownHero==b.ownHero&&a.allies==b.allies&&a.taken==b.taken;}
template<class R,class T>bool Stable(R& r,uintptr_t a,T& out){T b{};return r.Read(a,out)&&r.Read(a,b)&&out==b;}
template<class R>bool HeaderAt(R& r,uintptr_t base,int layout,Header& h){
 static const int countOff[]={0,16,48,80},dataOff[]={8,0,56,88};
 return Stable(r,base+countOff[layout],h.count)&&Stable(r,base+dataOff[layout],h.data)&&h.count>=1&&h.count<=64&&Ptr(h.data);
}
template<class R,class Known>Result Read(R& r,uintptr_t resource,int localPlayer,Known known){
 Result accepted;if(!Ptr(resource)||localPlayer<0||localPlayer>=64)return accepted;
 for(int pl=0;pl<4;++pl)for(int tl=0;tl<4;++tl){Header players,teams;
  if(!HeaderAt(r,resource+0x670,pl,players)||!HeaderAt(r,resource+0x608,tl,teams)||players.count!=teams.count||localPlayer>=players.count)continue;
  Result v;v.local=localPlayer;v.layoutPlayers=pl;v.layoutTeams=tl;
  std::array<int,64> team{},hero{};std::array<uint8_t,64> valid{};bool good=true;
  for(int i=0;i<players.count;++i){uintptr_t a=players.data+uintptr_t(i)*0xf0,b=teams.data+uintptr_t(i)*0x238;
   if(!Stable(r,a+0x30,valid[i])||valid[i]>1){good=false;break;}if(!valid[i])continue;
   if(!Stable(r,a+0x40,team[i])||team[i]<0||team[i]>13){good=false;break;}if(team[i]!=2&&team[i]!=3)continue;
   if(!Stable(r,b+0x98,hero[i])||hero[i]<0||(hero[i]!=0&&!known(hero[i]))){good=false;break;}
  }
  if(!good||!valid[localPlayer]||(team[localPlayer]!=2&&team[localPlayer]!=3))continue;v.team=team[localPlayer];v.ownHero=hero[localPlayer];int ownN=0,enemyN=0,allyN=0;
  for(int i=0;i<players.count;++i)if(valid[i]&&(team[i]==2||team[i]==3)){
   if(team[i]==v.team)++ownN;else ++enemyN;
   if(hero[i]){if(std::find(v.taken.begin(),v.taken.end(),hero[i])!=v.taken.end()){good=false;break;}v.taken.push_back(hero[i]);}
   if(i!=localPlayer&&team[i]==v.team){if(allyN>=4){good=false;break;}v.allies[allyN++]=hero[i];}
  }
  Header p2,t2;
  if(!good||ownN>5||enemyN>5||!HeaderAt(r,resource+0x670,pl,p2)||!HeaderAt(r,resource+0x608,tl,t2)||p2.count!=players.count||p2.data!=players.data||t2.count!=teams.count||t2.data!=teams.data)continue;
  v.ok=true;std::sort(v.taken.begin(),v.taken.end());
  if(accepted.ok&&!Equal(accepted,v))return {}; // Ambiguous layouts cannot choose a plausible-looking winner.
  accepted=v;
 }
 return accepted;
}
}
