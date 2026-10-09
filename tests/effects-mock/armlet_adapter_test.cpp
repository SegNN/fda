#include "armlet.cpp"
#include <cassert>
#include <iostream>
namespace game {Sys g_sys;uintptr_t probeHero=0;uint32_t probeHandle=0;uintptr_t EntityByHandle(uint32_t h){return h==probeHandle?probeHero:0;}}
static void PutReady(uintptr_t a,bool on){mem::Put(a+0x62D,uint8_t(on));mem::Put(a+0x638,0.f);mem::Put(a+0x619,uint8_t(1));mem::Put(a+0x634,uint8_t(0));mem::Put(a+0x630,0.f);mem::Put(a+0x654,uint8_t(0));mem::Put(a+0x655,uint8_t(0));}
int main(){
 Frame f;f.ok=f.localAlive=true;f.localHero=0x100000000ULL;f.localHandle=0x8001;f.rules=0x900000000ULL;f.localTeam=2;
 game::probeHero=f.localHero;game::probeHandle=f.localHandle;
 FrameUnit self;self.addr=f.localHero;self.entityHandle=f.localHandle;self.kind=UnitKind::Hero;self.alive=true;self.hp=300;strcpy(self.name,"npc_dota_hero_huskar");self.inventoryRead=self.buffsRead=true;self.itemN=1;
 auto& item=self.items[0];item.addr=0x200000000ULL;item.instanceHandle=0x8002;item.slot=0;strcpy(item.icon,"armlet");
 f.units={self};mem::Put(f.rules+0x38,uint8_t(0));game::g_sys.localCtrl=0x300000000ULL;uintptr_t list=0x400000000ULL;
 mem::Put(game::g_sys.localCtrl+0x9C8,1);mem::Put(game::g_sys.localCtrl+0x9D0,list);mem::Put(list,1);mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));
 mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,f.localHandle);
 uintptr_t identity=0x500000000ULL;mem::Put(item.addr+off::instEntity,identity);mem::Put(identity,item.addr);mem::Put(identity+off::idHandleFld,item.instanceHandle);
 mem::Put(self.addr+0x12B8,uint64_t(0));PutReady(item.addr,true);cfg::armletAuto=true;cfg::menuOpen=false;cfg::armletKey='Z';binds::items[11].vk='H';cfg::armletSlot=0;
 assert(!armlet::Tick(f)&&winmock::keyInputs==0);cfg::armletConfirmed=true;
 // Exact full handle payload is accepted, but a stale serial is not masked to an index.
 mem::Put(list,f.localHandle);assert(armlet::Selected(f.localHandle));assert(strstr(armlet::SelectionDiagnostics(),"match=1"));
 mem::Put(list,f.localHandle+0x4000);assert(!armlet::Selected(f.localHandle));assert(strstr(armlet::SelectionDiagnostics(),"match=0"));
 mem::Put(list,uint32_t(0x80008001));assert(armlet::Selected(0x80008001));
 mem::Put(list,1);
 // ARM9 standard CUtlVector: pointer +0, size +16, stable singleton selection.
 mem::Put(game::g_sys.localCtrl+0x9C8,list);mem::Put(game::g_sys.localCtrl+0x9D8,1);assert(armlet::Selected(f.localHandle));
 mem::Put(game::g_sys.localCtrl+0x9D8,2);assert(!armlet::Selected(f.localHandle));
 mem::Put(game::g_sys.localCtrl+0x9C8,1);mem::Put(game::g_sys.localCtrl+0x9D0,list);mem::Put(game::g_sys.localCtrl+0x9D8,0);
 // Menu enable must work without requiring a second activation hotkey.
 binds::items[11].vk=0;f.units[0].hp=1000;armlet::Tick(f);assert(strstr(armlet::Status(),"HP above threshold"));
 mem::Put(self.addr+0x12B8,killcore::Bit(3)|killcore::Bit(1));armlet::Tick(f);assert(strstr(armlet::Status(),"HP above threshold")&&winmock::keyInputs==0);
 mem::Put(self.addr+0x12B8,killcore::Bit(4));assert(!armlet::Tick(f)&&winmock::keyInputs==0);mem::Put(self.addr+0x12B8,uint64_t(0));
 f.units[0].hp=300;binds::items[11].vk='H';
 f.observedOnly=true;assert(!armlet::Tick(f)&&winmock::keyInputs==0);f.observedOnly=false;
 cfg::menuOpen=true;assert(!armlet::Tick(f));assert(strstr(armlet::Status(),"Close the menu"));assert(strstr(armlet::LastClosedStatus(),"Local player unresolved"));cfg::menuOpen=false;
 mem::Put(f.rules+0x38,uint8_t(1));assert(!armlet::Tick(f));mem::Put(f.rules+0x38,uint8_t(0));
 f.units[0].buffsRead=false;assert(!armlet::Tick(f));f.units[0].buffsRead=true;
 mem::Put(list,2);assert(!armlet::Tick(f));mem::Put(list,1);
 mem::Put(game::g_sys.localCtrl+0x9C8,2);assert(!armlet::Tick(f));mem::Put(game::g_sys.localCtrl+0x9C8,1);
 strcpy(f.units[0].name,"npc_dota_hero_lina");assert(!armlet::Tick(f));strcpy(f.units[0].name,"npc_dota_hero_huskar");
 f.units[0].items[0].slot=6;assert(!armlet::Tick(f));f.units[0].items[0].slot=0;
 winmock::foreground=nullptr;assert(!armlet::Tick(f));winmock::foreground=(HWND)1;
 mem::Put(self.addr+0x12B8,killcore::Bit(4));assert(!armlet::Tick(f));mem::Put(self.addr+0x12B8,uint64_t(0));
 mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0x8003));assert(!armlet::Tick(f));mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));
 buffreader::Buff b;b.address=0x600000000ULL;b.parent=f.localHandle;b.serial=7;b.index=3;strcpy(b.name,"modifier_viper_poison_attack_slow");
 mem::Put(b.address,uintptr_t(0x700000000ULL));rtti::classes[0x700000000ULL]="CDOTA_Modifier_Viper_PoisonAttack_Slow";mem::Put(b.address+0x74,b.parent);mem::Put(b.address+0x48,b.serial);mem::Put(b.address+0x50,b.index);mem::Put(b.address+0x1AA8,25.f);f.units[0].buffs={b};assert(!armlet::Tick(f));assert(strstr(armlet::Status(),"new OFF blocked"));assert(armlet::machine.State()==armletcore::Phase::Idle);f.units[0].buffs.clear();
 mem::Put(identity+off::idHandleFld,uint32_t(0xC002));assert(!armlet::Tick(f));mem::Put(identity+off::idHandleFld,item.instanceHandle);
 buffreader::Buff ownPassive;strcpy(ownPassive.name,"modifier_huskar_burning_spear_self");f.units[0].buffs={ownPassive};
 assert(winmock::keyInputs==0);assert(armlet::Tick(f)&&winmock::keyInputs==2);assert(strstr(armlet::DangerDiagnostics(),"matched=none"));
 f.units[0].buffs={b};f.units[0].hp=250;f.now=.05f;assert(armlet::Tick(f)&&winmock::keyInputs==2); // item still ON; no second key
 mem::Put(item.addr+0x62D,uint8_t(0));mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,item.instanceHandle);f.now=.11f;assert(armlet::Tick(f)&&winmock::keyInputs==4);
 mem::Put(item.addr+0x62D,uint8_t(1));buffreader::Buff effect;strcpy(effect.name,"modifier_item_armlet_unholy_strength");f.units[0].buffs.push_back(effect);f.now=.2f;assert(armlet::Tick(f));assert(strstr(armlet::DamageDiagnostics(),"candidate damage"));assert(strstr(armlet::DamageDiagnostics(),"predictionReady=0"));f.units[0].hp=800;
 for(int i=21;i<=130;++i){f.now=i*.01f;armlet::Tick(f);}assert(winmock::keyInputs==4);f.units[0].buffs.clear();
 f.now=1.4f;armlet::Tick(f);for(int i=141;i<=220;++i){f.now=i*.01f;armlet::Tick(f);}f.units[0].hp=300;f.now=2.3f;assert(armlet::Tick(f)&&winmock::keyInputs==6);
 winmock::foreground=nullptr;f.now=2.4f;assert(!armlet::Tick(f));winmock::foreground=(HWND)1;mem::Put(item.addr+0x62D,uint8_t(0));f.now=2.5f;assert(!armlet::Tick(f)&&winmock::keyInputs==6); // never replay ON after refocus
 assert(strstr(armlet::Status(),"may be OFF"));assert(strstr(armlet::CycleDiagnostics(),"Dota must have focus"));
 cfg::menuOpen=true;armlet::Tick(f);assert(strstr(armlet::CycleDiagnostics(),"Dota must have focus"));cfg::menuOpen=false;
 armlet::Reset();f.rules+=0x100;mem::Put(f.rules+0x38,uint8_t(0));assert(!armlet::Tick(f)&&!cfg::armletConfirmed&&winmock::keyInputs==6);
 // Initially OFF under Viper DoT: ON is permitted, no new OFF is generated.
 cfg::armletConfirmed=true;armlet::Reset();f.now=0;f.units[0].buffs={b};mem::Put(item.addr+0x62D,uint8_t(0));
 assert(armlet::Tick(f)&&winmock::keyInputs==8);assert(armlet::machine.State()==armletcore::Phase::WaitOn);
 armlet::Reset();mem::Put(item.addr+0x62D,uint8_t(1));assert(!armlet::Tick(f)&&winmock::keyInputs==8);assert(strstr(armlet::Status(),"new OFF blocked"));
 cfg::armletAuto=false;assert(!armlet::Tick(f));assert(winmock::clicks==0);
 const char* trace=armlet::TraceDiagnostics();assert(strstr(trace,"inputCalls=")&&strstr(trace,"deliveredPairs=")&&strstr(trace,"closedTick:")&&strstr(trace,"event["));
 assert(strstr(armlet::CycleDiagnostics(),"faultContext:")==nullptr); // no stale ACTIVE fault context after reset/idle
 // ARM18: real adapter, immediate ON after first verified ready OFF; no false recovery fault under fresh damage.
 cfg::armletAuto=cfg::armletConfirmed=true;cfg::armletThreshold=550;armlet::Reset();f.units[0].buffs={effect};f.units[0].hp=200;f.now=0;mem::Put(item.addr+0x62D,uint8_t(1));
 int before=winmock::keyInputs;assert(armlet::Tick(f)&&winmock::keyInputs==before+2);
 mem::Put(item.addr+0x62D,uint8_t(0));f.now=.02f;assert(armlet::Tick(f)&&winmock::keyInputs==before+4);
 mem::Put(item.addr+0x62D,uint8_t(1));f.units[0].hp=190;f.now=.04f;armlet::Tick(f);assert(armlet::machine.State()==armletcore::Phase::Recover);
 f.units[0].hp=180;for(int i=5;i<=110;++i){f.now=i*.01f;armlet::Tick(f);}assert(armlet::machine.State()==armletcore::Phase::Idle&&!strcmp(armlet::machine.FaultReason(),"none"));
 // User-report boundary: HP485 > 451.471 is NOT a trigger; HP404 is, unless menu is open.
 armlet::Reset();cfg::armletThreshold=451.471f;f.units[0].hp=485;f.now=2;before=winmock::keyInputs;armlet::Tick(f);assert(winmock::keyInputs==before&&strstr(armlet::TraceDiagnostics(),"hp=485 threshold=451.471 hpLEthreshold=0"));
 cfg::menuOpen=true;f.units[0].hp=404;f.now=2.1f;armlet::Tick(f);assert(winmock::keyInputs==before);cfg::menuOpen=false;armlet::Tick(f);assert(winmock::keyInputs==before+2);
 // A death/canceled pending cycle remains visible in history, never masquerading as the active fault after idle reset.
 f.localAlive=false;f.units[0].hp=0;f.now=2.2f;armlet::Tick(f);assert(armlet::machine.State()==armletcore::Phase::Fault);
 f.localAlive=true;f.units[0].hp=900;f.now=3.2f;armlet::Tick(f);assert(armlet::machine.State()==armletcore::Phase::Idle);assert(strstr(armlet::CycleDiagnostics(),"faultContext:")==nullptr&&strstr(armlet::TraceDiagnostics(),"historicalOnly_faultContext:"));
 mem::Put(self.addr+0x12B8,uint64_t(0x200000106));f.now=3.3f;armlet::Tick(f);assert(strstr(armlet::Status(),"Unit-state mask")&&strstr(armlet::TraceDiagnostics(),"blockedMask=0x200000100"));mem::Put(self.addr+0x12B8,uint64_t(0));
 cfg::armletAuto=false;
 cfg::armletAuto=true;armlet::Reset();assert(armlet::OwnsLane(f));strcpy(f.units[0].name,"npc_dota_hero_lina");assert(!armlet::OwnsLane(f));strcpy(f.units[0].name,"npc_dota_hero_huskar");cfg::armletAuto=false;assert(!armlet::OwnsLane(f));
 std::cout<<"PASS ARM18 production Armlet with MOCK memory/Windows: Viper DoT after OFF does not cancel ON; initially OFF under Viper may turn ON; new OFF blocked; first fault preserved; Huskar, selection, slot, focus/menu, mute/channel/DoT/unknown guards; OFF observation; no late toggle; no mouse actions\n";
}
