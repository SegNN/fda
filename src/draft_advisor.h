#pragma once
#include <vector>
#include <array>
#include <algorithm>
namespace draftadvisor {
struct Hero {const char* label;int id;std::array<int,9> roles;};
inline const Hero heroes[]={
#include "draft_roles.inc"
};
inline constexpr int count=sizeof(heroes)/sizeof(heroes[0]);
struct Suggestion {int index=0;float score=0;std::array<float,9> contributions{};};
// Coverage-only heuristic. No winrate, counterpick, lane, player-skill or patch-meta claims.
inline std::vector<Suggestion> Recommend(const std::array<int,4>& allies,const std::vector<int>& excluded={}){
 const std::array<float,9> target={4,4,3,4,0,3,0,2,3};std::array<float,9> cover{};
 for(int i:allies)if(i>=0&&i<count)for(int r=0;r<9;++r)cover[r]+=heroes[i].roles[r];
 std::vector<Suggestion> out;
 for(int i=0;i<count;++i){if(std::find(allies.begin(),allies.end(),i)!=allies.end()||std::find(excluded.begin(),excluded.end(),i)!=excluded.end())continue;
  Suggestion v;v.index=i;for(int r=0;r<9;++r){v.contributions[r]=std::min(float(heroes[i].roles[r]),std::max(0.f,target[r]-cover[r]));v.score+=v.contributions[r];}
  out.push_back(v);
 }
 std::stable_sort(out.begin(),out.end(),[](const Suggestion&a,const Suggestion&b){return a.score>b.score;});return out;
}
}
