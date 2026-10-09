#include "draft_lane_planner.h"
#include <cassert>
#include <set>
#include <iostream>
int main(){using namespace draftadvisor;
 for(const auto& row:positionWeights)for(int w:row)assert(w>=0&&w<=100);
 assert(std::string(LaneName(2,0,true))=="BOTTOM"&&std::string(LaneName(3,0,true))=="TOP");assert(std::string(LaneName(2,2,true))=="TOP"&&std::string(LaneName(3,2,true))=="BOTTOM");assert(std::string(LaneName(2,1,true))=="MID");
 int carry=IndexByName("npc_dota_hero_antimage"),support=IndexByName("npc_dota_hero_crystal_maiden"),mid=IndexByName("npc_dota_hero_invoker"),off=IndexByName("npc_dota_hero_axe");assert(carry>=0&&support>=0&&mid>=0&&off>=0);
 auto ranked=RecommendLanes({carry,mid,off,-1},{0,1,2,-1},4);assert(!ranked.empty());
 for(const auto& v:ranked){assert(v.position==4&&v.partner==0&&v.allyPositions[0]==0&&v.allyPositions[1]==1&&v.allyPositions[2]==2);assert(v.hero!=carry&&v.hero!=mid&&v.hero!=off);}
 assert(RecommendLanes({carry,mid,-1,-1},{0,0,-1,-1}).empty());assert(RecommendLanes({carry,carry,-1,-1},{-1,-1,-1,-1}).empty());
 auto solo=RecommendLanes({carry,support,-1,-1},{0,4,-1,-1},1);assert(!solo.empty());for(auto v:solo)assert(v.partner==-1&&v.position==1);
 auto blocked=RecommendLanes({carry,mid,off,-1},{0,1,2,-1},4,{ranked[0].hero});for(auto v:blocked)assert(v.hero!=ranked[0].hero);
 auto chosen=RecommendLanes({support,mid,off,-1},{4,1,2,-1},0,{carry},carry);assert(chosen.size()==1&&chosen[0].hero==carry&&chosen[0].partner==0);
 std::cout<<"PASS lane planner: data weights, role locks, ally exclusion, paired carry/support, solo mid, Radiant/Dire mirror, conflicts. Heuristic, not live player lanes.\n";
}
