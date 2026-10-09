#include "kill_stealer.h"
#include "local_visibility.h"
#include "kill_stealer_core.h"
#include "hook.h"
#include "imgui.h"
namespace killstealer {
static char current[64]={};
void Observe(const Frame& f){current[0]=0;if(f.ok)for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.addr==f.localHero){std::strncpy(current,u.name,63);current[63]=0;break;}}
const char* CurrentHero(){return current;}
bool BindCurrentHero(){if(!current[0])return false;std::strncpy(cfg::ksHeroName,current,63);cfg::ksHeroName[63]=0;cfg::ksQuickcastConfirmed=false;return true;}
bool Run(const Frame& f){
 static ULONGLONG last=0;
 int hold=binds::items[10].vk;
 if(!cfg::killStealer||!cfg::ksQuickcastConfirmed||!f.ok||f.observedOnly||!f.localAlive||cfg::menuOpen||hold<=0||hold>=256||
    !binds::items[10].holding||!(GetAsyncKeyState(hold)&0x8000))return false;
 int key=cfg::ksSpellKey;
 if(!((key>='A'&&key<='Z')||(key>='0'&&key<='9'))||key==hold)return false;
 HWND hwnd=GetForegroundWindow();DWORD pid=0;if(!hwnd||!IsWindowVisible(hwnd)||!GetWindowThreadProcessId(hwnd,&pid)||pid!=GetCurrentProcessId())return false;
 // Do not synthesize a spell while user holds modifiers or that spell key.
 for(int vk:{VK_SHIFT,VK_CONTROL,VK_MENU,key})if(GetAsyncKeyState(vk)&0x8000)return false;
 const FrameUnit* self=nullptr;for(const auto& u:f.units)if(u.addr==f.localHero&&u.kind==UnitKind::Hero)self=&u;
 if(!self||!self->buffsRead||!cfg::ksHeroName[0]||strcmp(self->name,cfg::ksHeroName))return false;
 uint64_t ownState=0;if(!mem::Read(f.localHero+0x12B8,ownState)||(ownState&killcore::CannotCast))return false;
 const AbilityInfo* spell=nullptr;for(int i=0;i<self->abilN;++i){if(self->abil[i].phase)return false;if(self->abil[i].slot==cfg::ksAbilitySlot)spell=&self->abil[i];}
 if(!spell||spell->level<=0||!spell->automationReady)return false;
 ULONGLONG now=GetTickCount64();if(now-last<1200)return false;
 const FrameUnit* target=nullptr;
 for(const auto& u:f.units){
  if(u.kind!=UnitKind::Hero||u.team==f.localTeam||!u.buffsRead)continue;
  uint64_t state=0;float resistance=0;bool stateRead=mem::Read(u.addr+0x12B8,state);
  bool resistanceRead=cfg::ksDamageType==2 || mem::Read(u.addr+(cfg::ksDamageType==0?0x1630:0x162C),resistance);
  bool visible=localvisibility::Get(f,u).visible&&u.invis<.01f;
  killcore::Input in{visible,u.alive,u.illusion,u.buffsRead,stateRead,resistanceRead,spell->automationReady,u.hp,f.mana,spell->mana,cfg::ksDamageType,
   u.dist,cfg::ksRange,cfg::ksDamage,resistance,cfg::ksMargin,ownState,state,killcore::Protected(u.buffs)};
  if(killcore::Lethal(in)&&(!target||u.hp<target->hp))target=&u;
 }
 if(!target)return false;
 // Coordinate must land inside the client area, not on OS chrome/another window.
 ImVec2 xy;if(!view::W2SRaw(Vec3{target->pos.x,target->pos.y,target->pos.z+70},xy))return false;
 RECT bounds{};if(!GetClientRect(hwnd,&bounds)||xy.x<16||xy.y<16||xy.x>=bounds.right-16||xy.y>=bounds.bottom-16)return false;
 POINT pos{(LONG)xy.x,(LONG)xy.y};if(!ClientToScreen(hwnd,&pos)||GetForegroundWindow()!=hwnd)return false;
 if(!SetCursorPos(pos.x,pos.y))return false;
 if(GetForegroundWindow()!=hwnd||cfg::menuOpen||!(GetAsyncKeyState(hold)&0x8000))return false;
 INPUT inputs[2]{};inputs[0].type=inputs[1].type=INPUT_KEYBOARD;inputs[0].ki.wVk=inputs[1].ki.wVk=(WORD)key;inputs[1].ki.dwFlags=KEYEVENTF_KEYUP;
 UINT sent=SendInput(2,inputs,sizeof(INPUT));
 if(sent==1)SendInput(1,&inputs[1],sizeof(INPUT)); // release a partially delivered key-down
 last=now;
 // Do not restore immediately: quickcast consumes the cursor at the engine's next input tick.
 // No normal-cast mouse click is synthesized; configuration must match actual quickcast-on-keydown.
 return sent!=0;
}
}
