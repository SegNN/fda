#include "combos.cpp"
#include <cassert>
#include <iostream>
#include <map>
namespace game {Sys g_sys;std::map<uint32_t,uintptr_t> handles;uintptr_t EntityByHandle(uint32_t h){return handles[h];}}
namespace view {bool W2SRaw(const Vec3& p,ImVec2& out){out={p.x,p.y};return true;}}
namespace armlet {bool lanePending=false;bool Pending(){return lanePending;}bool OwnsLane(const Frame&){return lanePending;}}
static uintptr_t selected=0x400000000ULL;
static Frame Fixture(int profile){combos::Reset();for(auto& c:combos::settings)c={};combos::InitializeSettings();auto& c=combos::settings[profile];c.enabled=c.confirmed=true;c.holdKey='B';for(int j=0;j<combos::profiles[profile].count;++j)c.keys[j]=combos::profiles[profile].steps[j].defaultKey;
 winmock::cursorWorks=true;winmock::nextInputResult=-1;winmock::keys.clear();winmock::keys['B']=0x8000;winmock::foreground=(HWND)1;winmock::now=1000;winmock::cursor={300,300};game::handles.clear();mem::bytes.clear();cfg::menuOpen=cfg::armletAuto=false;armlet::lanePending=false;
 Frame f;f.ok=f.localAlive=true;f.localHero=0x100000000ULL;f.localHandle=0x8001;f.localTeam=2;f.rules=0x900000000ULL;f.mana=1000;f.localPos={300,300,0};
 game::g_sys.localCtrl=0x300000000ULL;mem::Put(f.rules+0x38,uint8_t(0));mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,f.localHandle);mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));mem::Put(game::g_sys.localCtrl+0x9c8,1);mem::Put(game::g_sys.localCtrl+0x9d0,selected);mem::Put(selected,f.localHandle);
 FrameUnit self;self.addr=f.localHero;self.entityHandle=f.localHandle;self.kind=UnitKind::Hero;self.alive=self.buffsRead=self.stateRead=true;strcpy(self.name,combos::profiles[profile].hero);self.abilN=combos::profiles[profile].count;
 for(int j=0;j<self.abilN;++j){auto& a=self.abil[j];a.addr=0x500000000ULL+j*0x1000;a.entityHandle=0x8003+j;a.phaseRead=a.cooldownRead=a.automationReady=true;a.level=1;a.mana=100;strcpy(a.icon,combos::profiles[profile].steps[j].ability);game::handles[a.entityHandle]=a.addr;}
 FrameUnit enemy;enemy.kind=UnitKind::Hero;enemy.addr=0x200000000ULL;enemy.entityHandle=0x8002;enemy.alive=enemy.buffsRead=enemy.stateRead=enemy.invisRead=enemy.teamVisibilityRead=true;enemy.teamVisibilityMask=4;enemy.team=3;enemy.pos={300,300,0};game::handles[f.localHandle]=self.addr;game::handles[enemy.entityHandle]=enemy.addr;f.units={self,enemy};return f;
}
int main(){
 for(int profile=0;profile<combos::count;++profile){Frame f=Fixture(profile);int before=winmock::keyInputs;for(int j=0;j<combos::profiles[profile].count;++j){assert(combos::Run(f));assert(winmock::keyInputs==before+2*j);winmock::now+=60;assert(combos::Run(f));assert(winmock::keyInputs==before+2*(j+1));winmock::now+=20;combos::Run(f);assert(combos::machine.Step()==j);f.units[0].abil[j].cd=10;f.units[0].abil[j].automationReady=false;winmock::now+=20;combos::Run(f);assert(combos::machine.Step()==j+1);}
  assert(combos::machine.State()==combocore::Phase::Done);combos::Run(f);assert(winmock::keyInputs==before+2*combos::profiles[profile].count);winmock::keys['B']=0;assert(!combos::Run(f));assert(combos::machine.State()==combocore::Phase::Idle);
 }
 auto blocked=[](auto mutate){auto f=Fixture(0);int before=winmock::keyInputs;mutate(f);assert(combos::Run(f));assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before);winmock::now+=100;combos::Run(f);assert(winmock::keyInputs==before);};
 blocked([](Frame&){combos::settings[0].confirmed=false;});blocked([](Frame&){cfg::menuOpen=true;});blocked([](Frame& f){f.observedOnly=true;});blocked([](Frame&){armlet::lanePending=true;});blocked([](Frame&){winmock::foreground=nullptr;});blocked([](Frame&){winmock::keys[VK_SHIFT]=0x8000;});blocked([](Frame&){winmock::keys[VK_RBUTTON]=0x8000;});blocked([](Frame& f){f.localTeam=40;});blocked([](Frame& f){f.mana=1;});blocked([](Frame& f){f.units[1].pos={700,500,0};});blocked([](Frame& f){f.units[1].teamVisibilityMask=0;});blocked([](Frame& f){f.units[1].invis=1;});blocked([](Frame& f){f.units[0].unitState=killcore::Bit(6);});blocked([](Frame& f){f.units[0].abil[0].phaseRead=false;});blocked([](Frame& f){game::handles[f.units[0].abil[0].entityHandle]=0;});blocked([](Frame& f){mem::Put(f.rules+0x38,uint8_t(1));});blocked([](Frame& f){mem::Put(selected,f.localHandle+0x4000);});blocked([](Frame&){mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0x8007));});blocked([](Frame&){combos::settings[0].keys[0]='B';});
 {auto f=Fixture(0);int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;cfg::menuOpen=true;combos::Run(f);assert(winmock::keyInputs==before);cfg::menuOpen=false;combos::Run(f);assert(winmock::keyInputs==before);}
 {auto f=Fixture(0);int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;f.rules+=0x1000;combos::Run(f);assert(winmock::keyInputs==before);}
 {auto f=Fixture(0);combos::Run(f);winmock::now+=60;combos::Run(f);int before=winmock::keyInputs;winmock::now+=2100;combos::Run(f);assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before);}
 {auto f=Fixture(0);int before=winmock::keyInputs;winmock::cursorWorks=false;combos::Run(f);assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before);}
 {auto f=Fixture(0);int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;winmock::nextInputResult=1;combos::Run(f);assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before+2);combos::Run(f);assert(winmock::keyInputs==before+2);}
 {auto f=Fixture(0);int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;winmock::nextInputResult=0;combos::Run(f);assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before);}
 {auto f=Fixture(0);f.units[1].teamVisibilityMask=0;f.units[1].npcVisibilityRead=f.units[1].npcVisible=true;int before=winmock::keyInputs;assert(combos::Run(f));winmock::now+=60;assert(combos::Run(f)&&winmock::keyInputs==before+2);f.units[1].npcVisible=false;winmock::now+=20;combos::Run(f);assert(combos::machine.State()==combocore::Phase::Fault&&winmock::keyInputs==before+2);}
 {auto f=Fixture(0);cfg::armletAuto=true;int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;combos::Run(f);assert(winmock::keyInputs==before+2);}
 {auto f=Fixture(1);int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;combos::Run(f);f.units[0].abil[0].cd=10;winmock::now+=20;combos::Run(f);
  mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,f.units[0].abil[0].entityHandle);winmock::now+=20;combos::Run(f);assert(combos::machine.State()!=combocore::Phase::Fault&&winmock::keyInputs==before+2);
  mem::Put(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,uint32_t(0));winmock::now+=60;combos::Run(f);winmock::now+=60;combos::Run(f);assert(winmock::keyInputs==before+4);
 }
 std::cout<<"PASS ESP21 sequential combo waits for previous acknowledged animation before next input.\n";
 {auto f=Fixture(1);winmock::keys['B']=0;winmock::keys[VK_SPACE]=0x8000;combos::settings[1].holdKey=VK_SPACE;int before=winmock::keyInputs;combos::Run(f);winmock::now+=60;combos::Run(f);assert(winmock::keyInputs==before+2);}
 {auto f=Fixture(1);combos::settings[1].confirmed=false;combos::Run(f);assert(strstr(combos::LastOutcome(),"Confirm quickcast"));winmock::keys['B']=0;combos::Run(f);assert(strstr(combos::LastOutcome(),"Confirm quickcast"));}
 std::cout<<"PASS production combo adapter with MOCK Windows/memory: all 9 plans; selection/serial/focus/menu/modifier/channel/Armlet/visibility/identity/timeout guards. No live Dota validation.\n";
}
