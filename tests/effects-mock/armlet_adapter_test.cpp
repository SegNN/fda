#include "armlet.cpp"
#include <cassert>
#include <iostream>
namespace game {Sys g_sys;}
static void PutReady(uintptr_t a,bool on){mem::Put(a+0x62D,uint8_t(on));mem::Put(a+0x638,0.f);mem::Put(a+0x619,uint8_t(1));mem::Put(a+0x634,uint8_t(0));mem::Put(a+0x630,0.f);mem::Put(a+0x654,uint8_t(0));mem::Put(a+0x655,uint8_t(0));}
int main(){
 Frame f;f.ok=f.localAlive=true;f.localHero=0x100000000ULL;f.localHandle=0x8001;f.rules=0x900000000ULL;f.localTeam=2;
 FrameUnit self;self.addr=f.localHero;self.entityHandle=f.localHandle;self.kind=UnitKind::Hero;self.alive=true;self.hp=300;strcpy(self.name,"npc_dota_hero_huskar");self.inventoryRead=self.buffsRead=true;self.itemN=1;
 auto& item=self.items[0];item.addr=0x200000000ULL;item.instanceHandle=0x8002;item.slot=0;strcpy(item.icon,"armlet");
 f.units={self};mem::Put(f.rules+0x38,uint8_t(0));game::g_sys.localCtrl=0x300000000ULL;uintptr_t list=0x400000000ULL;
 mem::Put(game::g_sys.localCtrl+0x9C8,1);mem::Put(game::g_sys.localCtrl+0x9D0,list);mem::Put(list,1);mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));
 mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,f.localHandle);
 uintptr_t identity=0x500000000ULL;mem::Put(item.addr+off::instEntity,identity);mem::Put(identity,item.addr);mem::Put(identity+off::idHandleFld,item.instanceHandle);
 mem::Put(self.addr+0x12B8,uint64_t(0));PutReady(item.addr,true);cfg::armletAuto=true;cfg::menuOpen=false;cfg::armletKey='Z';binds::items[11].vk='H';cfg::armletSlot=0;
 assert(!armlet::Tick(f)&&winmock::keyInputs==0);cfg::armletConfirmed=true;
 cfg::menuOpen=true;assert(!armlet::Tick(f));cfg::menuOpen=false;
 mem::Put(f.rules+0x38,uint8_t(1));assert(!armlet::Tick(f));mem::Put(f.rules+0x38,uint8_t(0));
 f.units[0].buffsRead=false;assert(!armlet::Tick(f));f.units[0].buffsRead=true;
 mem::Put(list,2);assert(!armlet::Tick(f));mem::Put(list,1);
 mem::Put(game::g_sys.localCtrl+0x9C8,2);assert(!armlet::Tick(f));mem::Put(game::g_sys.localCtrl+0x9C8,1);
 strcpy(f.units[0].name,"npc_dota_hero_lina");assert(!armlet::Tick(f));strcpy(f.units[0].name,"npc_dota_hero_huskar");
 f.units[0].items[0].slot=6;assert(!armlet::Tick(f));f.units[0].items[0].slot=0;
 winmock::foreground=nullptr;assert(!armlet::Tick(f));winmock::foreground=(HWND)1;
 mem::Put(self.addr+0x12B8,killcore::Bit(1));assert(!armlet::Tick(f));mem::Put(self.addr+0x12B8,uint64_t(0));
 mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0x8003));assert(!armlet::Tick(f));mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));
 buffreader::Buff b;strcpy(b.name,"modifier_dazzle_poison_touch");f.units[0].buffs={b};assert(!armlet::Tick(f));f.units[0].buffs.clear();
 mem::Put(identity+off::idHandleFld,uint32_t(0xC002));assert(!armlet::Tick(f));mem::Put(identity+off::idHandleFld,item.instanceHandle);
 assert(winmock::keyInputs==0);assert(armlet::Tick(f)&&winmock::keyInputs==2);
 f.now=.05f;assert(armlet::Tick(f)&&winmock::keyInputs==2); // item still ON; no second key
 mem::Put(item.addr+0x62D,uint8_t(0));mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,item.instanceHandle);f.now=.11f;assert(armlet::Tick(f)&&winmock::keyInputs==4);
 mem::Put(item.addr+0x62D,uint8_t(1));f.now=.2f;assert(armlet::Tick(f));f.units[0].hp=800;
 for(int i=21;i<=130;++i){f.now=i*.01f;armlet::Tick(f);}assert(winmock::keyInputs==4);
 f.now=1.4f;armlet::Tick(f);for(int i=141;i<=220;++i){f.now=i*.01f;armlet::Tick(f);}f.units[0].hp=300;f.now=2.3f;assert(armlet::Tick(f)&&winmock::keyInputs==6);
 winmock::foreground=nullptr;f.now=2.4f;assert(!armlet::Tick(f));winmock::foreground=(HWND)1;mem::Put(item.addr+0x62D,uint8_t(0));f.now=2.5f;assert(!armlet::Tick(f)&&winmock::keyInputs==6); // never replay ON after refocus
 assert(strstr(armlet::Status(),"may be OFF"));
 armlet::Reset();f.rules+=0x100;mem::Put(f.rules+0x38,uint8_t(0));assert(!armlet::Tick(f)&&!cfg::armletConfirmed&&winmock::keyInputs==6);
 cfg::armletAuto=false;assert(!armlet::Tick(f));assert(winmock::clicks==0);
 std::cout<<"PASS production Armlet with MOCK memory/Windows: Huskar, selection, slot, focus/menu, mute/channel/DoT/unknown guards; OFF observation; no late toggle; no mouse actions\n";
}
