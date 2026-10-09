#include "../src/armlet_core.h"
#include <cassert>
#include <iostream>
using namespace armletcore;
int main(){
 Machine m;Input x;x.allowed=x.ready=x.on=x.effectRead=x.effectOn=true;x.hp=300;x.hero=0x8001;x.item=0x8002;x.threshold=350;
 assert(m.Tick(x)==Action::TurnOff&&m.State()==Phase::WaitOff);
 x.now=.05f;assert(m.Tick(x)==Action::None); // no blind double-tap
 x.on=false;x.now=.11f;assert(m.Tick(x)==Action::TurnOn);x.now=.2f;x.on=true;assert(m.Tick(x)==Action::None&&m.State()==Phase::Recover);
 x.hp=800;for(int i=21;i<=130;++i){x.now=i*.01f;assert(m.Tick(x)==Action::None);}assert(m.State()==Phase::Idle);
 x.hp=300;x.now=1.4f;assert(m.Tick(x)==Action::None);x.hp=800;x.now=1.5f;assert(m.Tick(x)==Action::None);
 for(int i=151;i<=210;++i){x.now=i*.01f;assert(m.Tick(x)==Action::None);}x.hp=300;x.now=2.2f;assert(m.Tick(x)==Action::TurnOff);
 x.allowed=false;x.now=2.3f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);x.allowed=true;x.on=false;x.now=2.4f;assert(m.Tick(x)==Action::None); // no late restore
 m.Reset();x.on=false;x.now=0;assert(m.Tick(x)==Action::TurnOn);x.now=.01f;x.on=true;assert(m.Tick(x)==Action::None);
 for(int i=2;i<=120;++i){x.now=i*.01f;m.Tick(x);}assert(m.State()==Phase::Idle); // active state restored despite no net HP increase
 m.Reset();x.on=true;x.now=0;assert(m.Tick(x)==Action::TurnOff);x.item++;x.now=.1f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.now=0;assert(m.Tick(x)==Action::TurnOff);x.now=.1f;m.Tick(x);x.now=.05f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.now=0;assert(m.Tick(x)==Action::TurnOff);x.now=.8f;assert(m.Tick(x)==Action::None&&m.State()==Phase::Fault);
 m.Reset();x.ready=false;x.now=0;assert(m.Tick(x)==Action::None);x.ready=true;x.hp=351;x.now=.1f;assert(m.Tick(x)==Action::None);
 // Rearm after verified recovery even when HP never exceeded threshold+100.
 Machine repeat;Input y;y.allowed=y.ready=y.on=y.effectRead=y.effectOn=true;y.hp=200;y.threshold=550;y.hero=0x8001;y.item=0x8002;
 assert(repeat.Tick(y)==Action::TurnOff);y.on=false;y.now=.11f;assert(repeat.Tick(y)==Action::TurnOn);
 y.on=true;y.hp=501;y.now=.2f;assert(repeat.Tick(y)==Action::None);
 for(int i=21;i<=130;++i){y.now=i*.01f;assert(repeat.Tick(y)==Action::None);}assert(repeat.State()==Phase::Idle);
 y.ready=false;for(int i=131;i<=219;++i){y.now=i*.01f;assert(repeat.Tick(y)==Action::None);}
 y.ready=true;y.hp=450;y.now=2.2f;assert(repeat.Tick(y)==Action::TurnOff);
 buffreader::Buff b;strcpy(b.name,"modifier_huskar_burning_spear");assert(!Dangerous({b}));strcpy(b.name,"modifier_huskar_burning_spear_debuff");assert(Dangerous({b}));strcpy(b.name,"modifier_item_armlet_unholy_strength");assert(!Dangerous({b}));strcpy(b.name,"modifier_dazzle_poison_touch");assert(Dangerous({b}));
 // ARM12 exact user-reported passive spell modifier is NOT an enemy DoT.
 buffreader::Buff own1,own2,own3;strcpy(own1.name,"modifier_huskar_blood_magic");strcpy(own2.name,"modifier_item_armlet");strcpy(own3.name,"modifier_huskar_burning_spear_self");
 assert(!Dangerous({own1,own2,own3})&&DangerousName({own1,own2,own3})==nullptr);
 strcpy(b.name,"modifier_huskar_burning_spear_debuff");assert(Dangerous({own3,b})&&!strcmp(DangerousName({own3,b}),b.name));
 strcpy(b.name,"modifier_huskar_burning_spear_self_unknown_damage");assert(Dangerous({b}));
 strcpy(b.name,"modifier_dazzle_poison_touch");assert(Dangerous({own3,b}));
 Machine guarded;Input z;z.allowed=z.ready=z.on=z.effectRead=z.effectOn=true;z.hp=200;z.threshold=400;z.hero=1;z.item=2;z.allowTurnOff=false;
 assert(guarded.Tick(z)==Action::None&&guarded.State()==Phase::Idle);
 z.on=false;z.now=.1f;assert(guarded.Tick(z)==Action::TurnOn); // initial OFF may turn ON under DoT
 guarded.Reset();z.on=true;z.allowTurnOff=true;z.now=0;assert(guarded.Tick(z)==Action::TurnOff);
 z.allowTurnOff=false;z.now=.05f;assert(guarded.Tick(z)==Action::None&&guarded.State()==Phase::WaitOff);
 z.on=false;z.now=.11f;assert(guarded.Tick(z)==Action::TurnOn); // risk appeared AFTER OFF request
 z.on=true;z.now=.2f;assert(guarded.Tick(z)==Action::None&&guarded.State()==Phase::Recover);
 guarded.Cancel("Focus lost");assert(!strcmp(guarded.FaultReason(),"Focus lost"));guarded.Failed("Later reason");assert(!strcmp(guarded.FaultReason(),"Focus lost"));
 guarded.Reset();assert(!strcmp(guarded.FaultReason(),"none"));
 // Confirmed OFF sends ON on the very first ready sample, with no extra 100ms delay.
 Machine fast;Input a;a.allowed=a.ready=a.on=a.effectRead=a.effectOn=true;a.hp=200;a.threshold=350;a.hero=1;a.item=2;
 assert(fast.Tick(a)==Action::TurnOff);a.now=.02f;a.on=false;assert(fast.Tick(a)==Action::TurnOn);
 a.now=.04f;a.on=true;a.hp=190;assert(fast.Tick(a)==Action::None&&fast.State()==Phase::Recover);
 for(int i=5;i<=110;++i){a.now=i*.01f;fast.Tick(a);}assert(fast.State()==Phase::Idle); // incoming damage is not a failed ON acknowledgement
 Machine noEffect;a.now=0;a.on=false;a.effectOn=false;assert(noEffect.Tick(a)==Action::TurnOn);
 a.on=true;a.now=.02f;noEffect.Tick(a);assert(noEffect.State()==Phase::WaitOn);
 for(int i=3;i<=72;++i){a.now=i*.01f;noEffect.Tick(a);}assert(noEffect.State()==Phase::Fault); // raw ON alone is insufficient
 Machine disappearing;a.now=0;a.on=false;a.effectOn=true;disappearing.Tick(a);a.now=.02f;a.on=true;disappearing.Tick(a);
 a.effectOn=false;for(int i=3;i<=110;++i){a.now=i*.01f;disappearing.Tick(a);}assert(disappearing.State()==Phase::Fault);
 std::cout<<"PASS ARM18 Armlet state machine: new-OFF veto without ON cancellation; first-fault reason; immediate ON after observed ready OFF, verified active effect despite net HP loss, readiness, threshold, recovery, spacing, cancellation, identity/time change, faults and recognized DoT\n";
}
