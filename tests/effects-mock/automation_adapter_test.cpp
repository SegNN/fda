#include "auto_accept.cpp"
#include "kill_stealer.cpp"
#include <cassert>
#include <iostream>
namespace view {bool W2SRaw(const Vec3& p,ImVec2& out){out={p.x,p.y};return true;}}
static void Ready(uint64_t id,int state=0){auto p=cosmetic::Pack(7170,cosmetic::pb::Encode({cosmetic::pb::F64(1,id),cosmetic::pb::V(6,state)}));autoaccept::Observe(p.type,p.data);}
int main(){
 cfg::menuOpen=false;cfg::autoAccept=true;autoaccept::Tick(false);assert(winmock::clicks==0);
 for(int i=0;i<6400;++i)for(int c=0;c<3;++c)winmock::scene[i*4+c]=(i%160/4)%2?240:10;
 winmock::now=1001;Ready(1);winmock::keys[VK_F8]=0x8001;binds::Edge[VK_F8]=(GetAsyncKeyState(VK_F8)&0x8000)!=0;autoaccept::Tick(false);binds::Edge[VK_F8]=false;assert(winmock::clicks==0);
 winmock::now=1002;Ready(2);autoaccept::Tick(false);winmock::now=1252;autoaccept::Tick(false);assert(winmock::clicks==0);winmock::now=1502;autoaccept::Tick(false);assert(winmock::clicks==1);
 winmock::now=1752;autoaccept::Tick(false);assert(winmock::clicks==1); // one attempt per invitation
 Ready(3,1);autoaccept::Tick(false);assert(winmock::clicks==1);
 winmock::now=2002;Ready(4);winmock::width=1024;autoaccept::Tick(false);assert(winmock::clicks==1);winmock::width=800;
 winmock::foreground=nullptr;autoaccept::Tick(false);assert(winmock::clicks==1);winmock::foreground=(HWND)1;
 winmock::captureWorks=false;autoaccept::Tick(false);assert(winmock::clicks==1);winmock::captureWorks=true;
 autoaccept::Tick(true);assert(winmock::clicks==1); // never during a valid match frame
 Frame f;f.ok=f.localAlive=true;f.localHero=0x100000000ULL;f.localTeam=2;f.mana=200;
 FrameUnit self;self.addr=f.localHero;self.kind=UnitKind::Hero;strcpy(self.name,"npc_dota_hero_lina");self.buffsRead=true;self.abilN=1;self.abil[0].slot=3;self.abil[0].level=1;self.abil[0].mana=100;self.abil[0].automationReady=true;f.units.push_back(self);
 FrameUnit enemy;enemy.kind=UnitKind::Hero;enemy.addr=0x200000000ULL;enemy.team=3;enemy.hp=50;enemy.alive=true;enemy.buffsRead=true;enemy.teamVisibilityRead=true;enemy.teamVisibilityMask=4;enemy.dist=400;enemy.pos={300,300,0};f.units.push_back(enemy);
 mem::Put(self.addr+0x12B8,uint64_t(0));mem::Put(enemy.addr+0x12B8,uint64_t(0));mem::Put(enemy.addr+0x1630,.25f);
 cfg::killStealer=true;cfg::ksDamage=200;cfg::ksRange=600;cfg::ksMargin=30;cfg::ksSpellKey='R';cfg::ksAbilitySlot=3;binds::items[10].vk='H';binds::items[10].holding=true;winmock::keys['H']=0x8000;winmock::now=10000;
 killstealer::Observe(f);assert(killstealer::BindCurrentHero());assert(!killstealer::Run(f)); // no quickcast confirmation
 cfg::ksQuickcastConfirmed=true;f.units[1].buffsRead=false;assert(!killstealer::Run(f));f.units[1].buffsRead=true;
 mem::Put(enemy.addr+0x12B8,killcore::Bit(8));assert(!killstealer::Run(f));mem::Put(enemy.addr+0x12B8,uint64_t(0));
 assert(killstealer::Run(f));assert(winmock::keyInputs==2);assert(!killstealer::Run(f));
 winmock::now=13000;strcpy(f.units[0].name,"npc_dota_hero_lion");assert(!killstealer::Run(f));assert(winmock::keyInputs==2);
 std::cout<<"PASS production auto-accept under MOCK GC/Windows/GDI: calibration, stability, dedup, dimension/focus/capture/match guards\n";
 std::cout<<"PASS production Kill Stealer under MOCK input: profile, hero lock, quickcast confirmation, buffs, immunity, cooldown\n";
}
