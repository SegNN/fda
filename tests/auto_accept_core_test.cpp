#include "../src/auto_accept_core.h"
#include <cassert>
#include <iostream>
using namespace cosmetic;
Packet Status(uint64_t id,int state){return Pack(7170,pb::Encode({pb::F64(1,id),pb::V(6,state)}));}
int main(){acceptcore::Gate g;auto p=Status(1,0);g.Observe(p.type,p.data,100);assert(!g.Fresh(100));g.Enable(true,100);g.Observe(p.type,p.data,100);assert(!g.Fresh(100));g.Observe(p.type,p.data,101);assert(g.Fresh(101));
 assert(!g.Sample(101,true,true,true,true,true));assert(!g.Sample(201,true,true,true,true,true));assert(!g.Sample(351,true,true,true,true,true));assert(g.Sample(601,true,true,true,true,true));g.Attempt();assert(!g.Fresh(602));auto accepted=Status(1,1);g.Observe(accepted.type,accepted.data,603);assert(g.Confirmed());
 p=Status(2,1);g.Observe(p.type,p.data,700);assert(!g.Fresh(700));p=Status(2,0);g.Observe(p.type,p.data,701);assert(!g.Sample(701,true,false,true,true,true));assert(!g.Sample(702,true,true,false,true,true));assert(!g.Sample(703,true,true,true,false,true));assert(!g.Sample(704,false,true,true,true,true));assert(!g.Fresh(9000));
 p=Pack(7170,pb::Encode({pb::F64(1,3)}));g.Observe(p.type,p.data,9001);assert(g.Fresh(9001)); // documented UNDECLARED default
 p=Pack(7170,pb::Encode({pb::F64(1,3),pb::V(6,0),pb::V(6,0)}));g.Observe(p.type,p.data,9002);assert(!g.Fresh(9002));
 p=Pack(7170,pb::Encode({pb::F64(1,4),pb::F64(6,0)}));g.Observe(p.type,p.data,9003);assert(!g.Fresh(9003));
 p=Pack(7170,pb::Encode({pb::F64(1,4),pb::V(6,0x100000000ULL)}));g.Observe(p.type,p.data,9004);assert(!g.Fresh(9004));
 acceptcore::Image a{160,40,std::vector<uint8_t>(19200,0)},b=a;assert(!acceptcore::Matches(a,b));for(int i=0;i<6400;++i){a.rgb[i*3]=a.rgb[i*3+1]=a.rgb[i*3+2]=(i%160/4)%2?240:10;}b=a;assert(acceptcore::Matches(a,b));b.rgb.assign(19200,255);assert(!acceptcore::Matches(a,b));
 for(int i=0;i<6400;++i){b.rgb[i*3]=0;b.rgb[i*3+1]=255;b.rgb[i*3+2]=0;}assert(!acceptcore::Informative(b)); // uniform colour isn't a button
 std::cout<<"PASS ready-up gating, schema default/state, freshness, duplicate fields, foreground, template and stable frames\n";
}
