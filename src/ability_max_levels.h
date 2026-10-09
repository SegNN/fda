#pragma once
#include <cstring>
// Exact named fallback from Valve public herodata, hero_id=59. Runtime valid override wins.
inline int PublishedAbilityMax(const char* name){
 if(!strcmp(name,"huskar_inner_fire"))return 4;
 if(!strcmp(name,"huskar_burning_spear"))return 4;
 if(!strcmp(name,"huskar_berserkers_blood"))return 4;
 if(!strcmp(name,"huskar_blood_magic"))return 1;
 if(!strcmp(name,"huskar_life_break"))return 3;
 return 0;
}
