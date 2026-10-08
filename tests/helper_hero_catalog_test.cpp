#include <set>
#include <string>
#include <cassert>
#include <iostream>
struct Entry{const char* label;const char* engine;int id;};
static Entry entries[]={
#include "../src/helper_hero_catalog.inc"
};
int main(){std::set<std::string> names;std::set<int> ids;int huskar=0;for(auto e:entries){assert(e.label[0]&&names.insert(e.engine).second&&ids.insert(e.id).second);if(std::string(e.engine)=="npc_dota_hero_huskar")++huskar;}assert(names.size()==127&&huskar==1);std::cout<<"PASS 127 unique source heroes and exactly one Huskar entry\n";}
