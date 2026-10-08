#pragma once
#include <cmath>
namespace damageestimate {
struct Range { float low=0,high=0; bool valid=false; };
inline Range Physical(int low,int high,int bonus,float armor){
    if(low<0||high<low||high>100000||bonus < -100000||bonus>100000||!std::isfinite(armor)||std::abs(armor)>10000)return {};
    float mult=1.f-(.06f*armor)/(1.f+.06f*std::abs(armor));
    float lo=(float)low+bonus,hi=(float)high+bonus;
    if(lo<=0||hi<lo)return {};
    return {lo*mult,hi*mult,true};
}
// This is base physical damage only, not a guarantee against modifiers/server timing.
inline bool Lethal(int hp,const Range& range,bool conservative=true){
    return hp>0 && range.valid && (float)hp <= (conservative?std::floor(range.low):(range.low+range.high)*.5f);
}
}
