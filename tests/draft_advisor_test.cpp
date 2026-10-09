#include "draft_advisor.h"
#include <cassert>
#include <set>
#include <string>
#include <iostream>
int main(){using namespace draftadvisor;std::set<int> ids;std::set<std::string> names;for(const auto& h:heroes){assert(ids.insert(h.id).second&&names.insert(h.label).second);for(int role:h.roles)assert(role>=0&&role<=3);}
 auto r=Recommend({0,1,2,3},{4,5});assert(r.size()==count-6);for(auto s:r){assert(s.index>5&&s.score>=0);float sum=0;for(float c:s.contributions)sum+=c;assert(sum==s.score);}
 for(size_t i=1;i<r.size();++i)assert(r[i-1].score>=r[i].score);
 auto invalid=Recommend({-1,-2,count,count+2});assert(invalid.size()==count);
 assert(Recommend({-1,-1,-1,-1})[0].index==invalid[0].index);
 std::cout<<"PASS draft advisor: unique roster, published role ranges, ally/exclusion filter, score attribution, stable ranking, invalid input.\n";
}
