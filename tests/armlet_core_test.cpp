#include "../src/armlet_core.h"
#include <cassert>
#include <iostream>
using namespace armletcore;
int main(){
 Machine m;Input x;x.allowed=x.ready=x.on=true;x.hp=300;x.hero=0x8001;x.item=0x8002;x.threshold=350;
 assert(m.Tick(x)==Action::TurnOff&&m.State()==Phase::WaitOff);
 x.now=.05f;assert(m.Tick(x)==Action::None); // no blind double-tap
 x.on=false;x.now=.11f;assert(m.Tick(x)==Action::TurnOn);x.now=.2f;x.on=true;assert(m.Tick(x)==Action::None&&m.State()==Phase::Recover);
 x.hp=800;for(int i=21;i<=130;++i){x.now=i*.01f;assert(m.Tick(x)==Action::None);}assert(m.State()==Phase::Idle);
 x.hp=300;x.now=1.4f;assert(m.Tick(x)==Action::None);x.hp=800;x.now=1.5f;assert(m.Tick(x)==Action::None);
 for(int i=151;i<=210;++i){x.now=i*.01f;assert(m.Tick(x)==Action::None);}x.hp=300;x.now=2.2f;assert(m.Tick(x)==Action::TurnOff);
 x.allowed=false;x.now=2.3f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);x.allowed=true;x.on=false;x.now=2.4f;assert(m.Tick(x)==Action::None); // no late restore
 m.Reset();x.on=false;x.now=0;assert(m.Tick(x)==Action::TurnOn);x.now=.01f;x.on=true;assert(m.Tick(x)==Action::None);
 for(int i=2;i<=120;++i){x.now=i*.01f;m.Tick(x);}assert(m.State()==Phase::Fault); // no HP increase
 m.Reset();x.on=true;x.now=0;assert(m.Tick(x)==Action::TurnOff);x.item++;x.now=.1f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.now=0;assert(m.Tick(x)==Action::TurnOff);x.now=.1f;m.Tick(x);x.now=.05f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.now=0;assert(m.Tick(x)==Action::TurnOff);x.now=.8f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.ready=false;x.now=0;assert(m.Tick(x)==Action::None);x.ready=true;x.hp=351;x.now=.1f;assert(m.Tick(x)==Action::None);
 buffreader::Buff b;strcpy(b.name,"modifier_huskar_burning_spear");assert(!Dangerous({b}));strcpy(b.name,"modifier_huskar_burning_spear_debuff");assert(Dangerous({b}));strcpy(b.name,"modifier_item_armlet_unholy_strength");assert(!Dangerous({b}));strcpy(b.name,"modifier_dazzle_poison_touch");assert(Dangerous({b}));
 std::cout<<"PASS Armlet state machine: observed OFF before ON, readiness, threshold, recovery, spacing, cancellation, identity/time change, faults and recognized DoT\n";
}
