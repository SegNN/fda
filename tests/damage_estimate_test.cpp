#include "../src/damage_estimate.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace damageestimate;
int main(){
 auto r=Physical(50,70,0,0);assert(r.valid&&r.low==50&&r.high==70);
 assert(Lethal(50,r));assert(!Lethal(60,r));assert(Lethal(60,r,false));
 assert(!Lethal(0,r));r=Physical(50,70,10,10);assert(r.valid&&r.low<60&&r.high<80);
 assert(!Lethal(40,r));assert(Lethal(37,r));
 r=Physical(50,70,0,-10);assert(r.valid&&r.low>50);
 assert(!Physical(70,50,0,0).valid);assert(!Physical(0,0,0,0).valid);
 assert(!Physical(50,70,-80,0).valid);assert(!Physical(50,70,0,std::numeric_limits<float>::quiet_NaN()).valid);
 assert(!Lethal(1,{}));std::cout<<"PASS: lower-bound, average, armor, invalid and unknown damage cases\n";
}
