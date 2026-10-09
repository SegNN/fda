#pragma once
#include "draft_advisor.h"
#include <cstring>
namespace draftadvisor {
inline const std::array<int,5> positionWeights[]={
#include "draft_position_weights.inc"
};
inline const char* LaneName(int team,int pos,bool en){
 if(pos==1)return en?"MID":"МИД";
 bool safe=pos==0||pos==4;bool bottom=(team==2)==safe;
 return bottom?(en?"BOTTOM":"НИЗ"):(en?"TOP":"ТОП");
}
inline int IndexById(int id){for(int i=0;i<count;++i)if(heroes[i].id==id)return i;return -1;}
inline int IndexByName(const char* name){struct Name{const char* label;const char* name;int id;};static const Name names[]={
#include "helper_hero_catalog.inc"
};for(int i=0;i<count;++i)if(name&&!strcmp(names[i].name,name))return i;return -1;}
struct LaneSuggestion {int hero=-1,position=-1,partner=-1;std::array<int,4> allyPositions{{-1,-1,-1,-1}};float score=-1e9f;};
// Slot assignment is a suggestion, not proof of allies' chosen lanes. Explicit locks override inference.
inline std::vector<LaneSuggestion> RecommendLanes(std::array<int,4> allies,std::array<int,4> locks,int requested=-1,const std::vector<int>& excluded={},int onlyHero=-1){
 std::vector<LaneSuggestion> out;std::array<int,5> permutation{{0,1,2,3,4}};
 for(int i=0;i<4;++i){if(allies[i]<-1||allies[i]>=count||locks[i]<-1||locks[i]>4)return {};
  for(int j=0;j<i;++j)if(allies[i]>=0&&allies[i]==allies[j])return {};}
 for(int hero=0;hero<count;++hero){if(onlyHero>=0&&hero!=onlyHero)continue;if(std::find(allies.begin(),allies.end(),hero)!=allies.end()||(hero!=onlyHero&&std::find(excluded.begin(),excluded.end(),hero)!=excluded.end()))continue;
  LaneSuggestion best;best.hero=hero;permutation={{0,1,2,3,4}};
  do {int mine=permutation[4];if((requested>=0&&mine!=requested)||positionWeights[hero][mine]<=0)continue;
   float score=float(positionWeights[hero][mine]);bool valid=true;int partner=-1;
   for(int i=0;i<4;++i){if(allies[i]<0)continue;if(locks[i]>=0&&permutation[i]!=locks[i]){valid=false;break;}
    if(positionWeights[allies[i]][permutation[i]]<=0){valid=false;break;}score+=positionWeights[allies[i]][permutation[i]];
    if((mine==0&&permutation[i]==4)||(mine==4&&permutation[i]==0)||(mine==2&&permutation[i]==3)||(mine==3&&permutation[i]==2))partner=i;
   }
   if(!valid)continue;
   if(partner>=0){const auto& a=heroes[allies[partner]].roles;const auto& h=heroes[hero].roles;
    score+=3.f*std::min(4,a[3]+h[3]); // Readable reason: lane pair control, not a win probability.
    score+=2.f*std::min(3,a[2]+h[2]);
   }
   if(score>best.score){best.score=score;best.position=mine;best.partner=partner;for(int i=0;i<4;++i)best.allyPositions[i]=allies[i]>=0?permutation[i]:-1;}
  }while(std::next_permutation(permutation.begin(),permutation.end()));
  if(best.position>=0)out.push_back(best);
 }
 std::stable_sort(out.begin(),out.end(),[](const LaneSuggestion&a,const LaneSuggestion&b){return a.score>b.score;});return out;
}
}
