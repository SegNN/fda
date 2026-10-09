#include "combos.h"
#include "local_visibility.h"
#include "hook.h"
#include "armlet.h"
#include "kill_stealer_core.h"
#include "imgui.h"
#include <utility>
#include <initializer_list>
namespace combos {
static combocore::Machine machine;static int active=-1;static const char* status="OFF";static HWND aimWindow=nullptr;static char lastOutcome[512]="No sequence completed or failed yet.";static uintptr_t matchRules=0;static double waitCastingSince=-1;
void InitializeSettings(){static bool once=false;if(once)return;once=true;for(int i=0;i<count;++i)for(int j=0;j<profiles[i].count;++j)settings[i].keys[j]=profiles[i].steps[j].defaultKey;}
const char* Status(){return status;}
const char* LastOutcome(){return lastOutcome;}
void Reset(){machine.Reset();active=-1;aimWindow=nullptr;matchRules=0;waitCastingSince=-1;status="Release / idle";}
void Cancel(const char* why){machine.Stop(why);status=why;}
static bool Key(int key){return (key>='A'&&key<='Z')||(key>='0'&&key<='9')||key==VK_SPACE;}
static bool Selected(uint32_t handle){uintptr_t ctrl=game::g_sys.localCtrl;
 for(auto layout:{std::pair<int,int>{0,8},{16,0}}){int n=0,n2=0,value=-1,again=-1;uintptr_t ptr=0,p2=0;
  if(mem::Read(ctrl+0x9c8+layout.first,n)&&n==1&&mem::Read(ctrl+0x9c8+layout.second,ptr)&&mem::ValidPtr(ptr)&&mem::Read(ptr,value)&&
   mem::Read(ctrl+0x9c8+layout.first,n2)&&n2==1&&mem::Read(ctrl+0x9c8+layout.second,p2)&&p2==ptr&&mem::Read(ptr,again)&&again==value&&
   (uint32_t(value)==handle||(value>=0&&uint32_t(value)==(handle&game::g_sys.handleMask))))return true;}
 return false;
}
static bool Send(HWND hwnd,int key){if(GetForegroundWindow()!=hwnd)return false;INPUT input[2]{};input[0].type=input[1].type=INPUT_KEYBOARD;
 input[0].ki.wVk=input[1].ki.wVk=WORD(key);input[1].ki.dwFlags=KEYEVENTF_KEYUP;UINT sent=SendInput(2,input,sizeof(INPUT));if(sent==1)SendInput(1,&input[1],sizeof(INPUT));return sent==2;}
bool Run(const Frame& f){InitializeSettings();
 const FrameUnit* self=nullptr;for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.addr==f.localHero&&u.entityHandle==f.localHandle)self=&u;
 int chosen=-1;for(int i=0;i<count;++i)if(self&&!strcmp(self->name,profiles[i].hero)&&settings[i].enabled)chosen=i;
 if(chosen<0){if(active>=0){machine.Reset();active=-1;}status="No enabled module for current hero";return false;}
 auto& config=settings[chosen];const auto& profile=profiles[chosen];
 bool held=Key(config.holdKey)&&(GetAsyncKeyState(config.holdKey)&0x8000);
 if(!held){Reset();status="Hold binding to start one sequence";return false;}
 if(active>=0&&active!=chosen){Cancel("Module/hero changed; release binding");return true;}active=chosen;
 auto block=[&](const char* why){snprintf(lastOutcome,sizeof(lastOutcome),"%s | step %d/%d | %s",profile.label,machine.Step()+1,profile.count,why);Cancel(why);return true;};
 if(!config.confirmed)return block("Confirm quickcast keys after testing in demo");
 if(f.localTeam!=2&&f.localTeam!=3)return block("Invalid local team");
 if(matchRules&&matchRules!=f.rules)return block("Match changed; release binding");matchRules=f.rules;
 if(!f.ok||f.observedOnly||!f.localAlive||!f.localHandle||cfg::menuOpen||armlet::OwnsLane(f))return block("Frame/menu/Armlet input-lane guard");
 if(config.holdKey==cfg::menuKey||config.holdKey==cfg::unloadKey)return block("Activation binding conflicts with menu/unload");
 for(int j=0;j<profile.count;++j){if(!Key(config.keys[j])||config.keys[j]==config.holdKey||config.keys[j]==cfg::menuKey||config.keys[j]==cfg::unloadKey)return block("Native spell key invalid/conflicting");
  for(int k=0;k<j;++k)if(config.keys[k]==config.keys[j])return block("Spell keys must be distinct");}
 HWND hwnd=GetForegroundWindow();DWORD pid=0;if(!hwnd||!IsWindowVisible(hwnd)||!GetWindowThreadProcessId(hwnd,&pid)||pid!=GetCurrentProcessId())return block("Dota not focused");
 for(int key:{VK_SHIFT,VK_CONTROL,VK_MENU,VK_LBUTTON,VK_RBUTTON})if(GetAsyncKeyState(key)&0x8000)return block("User modifier/mouse button held");
 uint8_t paused=2;uint32_t assigned=0;
 if(!mem::Read(f.rules+0x38,paused)||paused!=0||!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned)||assigned!=f.localHandle||game::EntityByHandle(f.localHandle)!=f.localHero||!Selected(f.localHandle))return block("Pause/owner/selection guard");
 if(!self||self->illusion||!self->buffsRead||!self->stateRead||(self->unitState&killcore::CannotCast))return block("Local state/modifiers unavailable or blocked");
 if(machine.State()==combocore::Phase::Done||machine.State()==combocore::Phase::Fault){status=machine.Status();snprintf(lastOutcome,sizeof(lastOutcome),"%s | step %d/%d | %s",profile.label,std::min(machine.Step()+1,profile.count),profile.count,status);return true;}
 const FrameUnit* target=nullptr;float best=100.f*100.f;POINT mouse{};if(!GetCursorPos(&mouse)||!ScreenToClient(hwnd,&mouse))return block("Cursor unavailable");
 for(const auto& u:f.units){if(u.kind!=UnitKind::Hero||!u.alive||u.team==f.localTeam||u.illusion||!u.entityHandle||!u.buffsRead||!u.stateRead||!u.invisRead||u.invis>.01f||
   !localvisibility::Get(f,u).visible||(u.unitState&killcore::CannotTarget)||killcore::Protected(u.buffs)||game::EntityByHandle(u.entityHandle)!=u.addr)continue;
  if(machine.Target()){if(u.entityHandle==machine.Target())target=&u;continue;}ImVec2 screen;if(!view::W2SRaw(Vec3{u.pos.x,u.pos.y,u.pos.z+70},screen))continue;
  if(!std::isfinite(screen.x)||!std::isfinite(screen.y))continue;
  float dx=screen.x-float(mouse.x),dy=screen.y-float(mouse.y),d=dx*dx+dy*dy;if(d<best){best=d;target=&u;}}
 if(!target)return block("No valid visible target near cursor / locked target lost");
 int step=machine.Step();if(step>=profile.count)return block("Invalid step index");const auto& plan=profile.steps[step];const AbilityInfo* spell=nullptr;
 for(int i=0;i<self->abilN;++i)if(!strcmp(self->abil[i].icon,plan.ability)){if(spell)return block("Ambiguous spell name");spell=&self->abil[i];}
 if(!spell||!spell->entityHandle||game::EntityByHandle(spell->entityHandle)!=spell->addr||!spell->cooldownRead||!spell->phaseRead)return block("Spell identity/cooldown/phase unreadable");
 if(machine.State()!=combocore::Phase::AwaitCast){
  uint32_t casting=0;if(!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,casting))return block("Active ability unreadable");
  bool busy=casting&&casting!=0xffffffffu;
  for(int i=0;i<self->abilN;++i){if(!self->abil[i].phaseRead)return block("Ability phase unreadable; release binding");busy=busy||self->abil[i].phase;}
  if(busy){
   if(machine.Step()==0)return block("Active ability/channel guard");
   double now=double(GetTickCount64())/1000.;if(waitCastingSince<0)waitCastingSince=now;
   if(now-waitCastingSince>2.||now<waitCastingSince)return block("Previous cast still active after 2s; release binding");
   status="Waiting previous cast animation; no extra input";return true;
  }
  waitCastingSince=-1;
 }
 int native=config.keys[step];if(GetAsyncKeyState(native)&0x8000)return block("User holds spell key");
 ImVec2 screen;RECT bounds{};POINT desired{};
 if(!view::W2SRaw(Vec3{target->pos.x,target->pos.y,target->pos.z+70},screen)||!std::isfinite(screen.x)||!std::isfinite(screen.y)||!GetClientRect(hwnd,&bounds)||screen.x<16||screen.y<16||screen.x>=bounds.right-16||screen.y>=bounds.bottom-16)return block("Target projection outside client");
 desired={(LONG)screen.x,(LONG)screen.y};if(!ClientToScreen(hwnd,&desired))return block("Client coordinate conversion failed");
 combocore::Input in;in.held=true;in.valid=true;in.hero=f.localHandle;in.target=target->entityHandle;in.spell=spell->entityHandle;in.now=double(GetTickCount64())/1000.;
 in.spellExists=true;in.ready=spell->automationReady&&spell->level>0;in.cooldown=spell->cd;in.cost=spell->mana;in.mana=f.mana;in.inRange=VecDist(f.localPos,target->pos)<=plan.range;
 auto action=machine.Tick(in,profile.count);status=machine.Status();
 if(action==combocore::Action::Aim){aimWindow=hwnd;machine.Submitted(SetCursorPos(desired.x,desired.y));status=machine.Status();}
 if(action==combocore::Action::Cast){if(hwnd!=aimWindow||GetForegroundWindow()!=hwnd||!(GetAsyncKeyState(config.holdKey)&0x8000)||cfg::menuOpen)return block("Context changed before input");
  uint8_t livePause=2;uint32_t liveOwner=0;
  if(game::EntityByHandle(f.localHandle)!=f.localHero||game::EntityByHandle(target->entityHandle)!=target->addr||game::EntityByHandle(spell->entityHandle)!=spell->addr||
     !mem::Read(f.rules+0x38,livePause)||livePause!=0||!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,liveOwner)||liveOwner!=f.localHandle||!Selected(f.localHandle))return block("Identity/owner/pause changed before input");
  if(!SetCursorPos(desired.x,desired.y))return block("Cursor move failed");machine.Submitted(Send(hwnd,native));status=machine.Status();}
 return true;
}
}
