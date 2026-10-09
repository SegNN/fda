#include "../src/combo_core.h"
#include "../src/combo_profiles.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
using namespace combocore;
static Input Ready(){Input x;x.held=x.valid=x.spellExists=x.ready=x.inRange=true;x.hero=1;x.target=2;x.spell=3;x.mana=500;x.cost=100;x.now=1;return x;}
int main(){
 for(const auto& p:combos::profiles){assert(p.count>=1&&p.count<=3);Machine m;auto x=Ready();
  for(int j=0;j<p.count;++j){assert(p.steps[j].range>0&&p.steps[j].ability&&p.steps[j].defaultKey);x.spell=3+j;x.cooldown=0;x.ready=true;x.now=1+j*.5;
   assert(m.Tick(x,p.count)==Action::Aim);assert(m.Step()==j);x.now+=.02;assert(m.Tick(x,p.count)==Action::None);x.now+=.04;assert(m.Tick(x,p.count)==Action::Cast);
   x.now+=.02;assert(m.Tick(x,p.count)==Action::None);assert(m.Step()==j);x.cooldown=5;x.ready=false;x.now+=.02;assert(m.Tick(x,p.count)==Action::None);assert(m.Step()==j+1);
  }
  assert(m.State()==Phase::Done);x.cooldown=0;x.ready=true;x.now+=3;assert(m.Tick(x,p.count)==Action::None);x.held=false;m.Tick(x,p.count);assert(m.State()==Phase::Idle&&m.Target()==0);
 }
 auto fault=[](Input bad){Machine m;m.Tick(bad,3);assert(m.State()==Phase::Fault);bad=Ready();assert(m.Tick(bad,3)==Action::None);bad.held=false;m.Tick(bad,3);assert(m.State()==Phase::Idle);};
 auto x=Ready();x.valid=false;fault(x);x=Ready();x.target=0;fault(x);x=Ready();x.now=std::numeric_limits<double>::quiet_NaN();fault(x);x=Ready();x.spellExists=false;fault(x);x=Ready();x.ready=false;fault(x);x=Ready();x.inRange=false;fault(x);x=Ready();x.mana=0;fault(x);x=Ready();x.cooldown=-1;fault(x);
 {Machine m;x=Ready();m.Tick(x,3);x.now=1.4;m.Tick(x,3);assert(m.State()==Phase::Fault);}
 {Machine m;x=Ready();m.Tick(x,3);x.now=.9;m.Tick(x,3);assert(m.State()==Phase::Fault);}
 {Machine m;x=Ready();m.Tick(x,3);x.target++;x.now+=.06;m.Tick(x,3);assert(m.State()==Phase::Fault);}
 {Machine m;x=Ready();m.Tick(x,3);x.now+=.06;m.Tick(x,3);x.now+=2.01;m.Tick(x,3);assert(m.State()==Phase::Fault);}
 {Machine m;x=Ready();m.Tick(x,3);x.now+=.06;m.Tick(x,3);x.spell++;x.cooldown=10;m.Tick(x,3);assert(m.State()==Phase::Fault&&m.Step()==0);}
 {Machine m;x=Ready();m.Tick(x,3);m.Submitted(false);assert(m.State()==Phase::Fault);}
 std::cout<<"PASS combo core: all 9 plans progress only after exact-spell cooldown; one per hold; identity/readiness/range/mana/clock/timeout/delivery guards\n";
}
