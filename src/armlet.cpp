#include "armlet.h"
#include "armlet_core.h"
namespace armlet {
static armletcore::Machine machine;
static const char* status="OFF / no input";
static uintptr_t contextRules=0;static uint32_t contextHero=0;
const char* Status(){return status;}
bool Pending(){return machine.Pending();}
void Reset(){machine.Reset();status="Reset. Verify native Armlet key, slot and state manually.";}
static bool Block(const char* why){machine.Cancel();status=machine.State()==armletcore::Phase::Fault?"Cycle stopped. Armlet may be OFF; check manually and reset.":why;return false;}
static bool Selected(uint32_t hero){
 uintptr_t ctrl=game::g_sys.localCtrl,data=0;int n=0,index=-1,check=0;uintptr_t again=0;
 // Schema: m_nSelectedUnits CUtlVector<CEntityIndex> at +0x9C8.
 // Assumed count/data placement; reject inconsistent reads. Live layout unverified.
 if(!mem::ValidPtr(ctrl)||!mem::Read(ctrl+0x9C8,n)||n!=1||!mem::Read(ctrl+0x9D0,data)||!mem::ValidPtr(data)||!mem::Read(data,index)||index<0)return false;
 if(!mem::Read(ctrl+0x9C8,check)||!mem::Read(ctrl+0x9D0,again)||n!=check||data!=again)return false;
 return uint32_t(index)==(hero&off::handleMask);
}
bool Tick(const Frame& f){
 if(!cfg::armletAuto)return Block("OFF");
 if(!cfg::armletConfirmed)return Block("Confirm native key and risk after testing in demo.");
 int key=cfg::armletKey,bind=binds::items[11].vk;
 if(!((key>='A'&&key<='Z')||(key>='0'&&key<='9'))||bind<=0||bind>=256||key==bind||key==cfg::menuKey||key==cfg::unloadKey||bind==cfg::menuKey||bind==cfg::unloadKey)return Block("Assign separate activation binding and native item key.");
 if(cfg::menuOpen||!f.ok||!f.localAlive||!f.localHandle||!std::isfinite(f.now))return Block("Menu / frame / life guard");
 uint8_t paused=2;if(!mem::ValidPtr(f.rules)||!mem::Read(f.rules+0x38,paused)||paused!=0)return Block("Pause state unknown / game paused.");
 HWND hwnd=GetForegroundWindow();DWORD pid=0;if(!hwnd||!IsWindowVisible(hwnd)||!GetWindowThreadProcessId(hwnd,&pid)||pid!=GetCurrentProcessId())return Block("Dota must have focus.");
 for(int vk:{VK_SHIFT,VK_CONTROL,VK_MENU,key})if(GetAsyncKeyState(vk)&0x8000)return Block("User key/modifier held.");
 const FrameUnit* self=nullptr;for(const auto& u:f.units)if(u.addr==f.localHero&&u.kind==UnitKind::Hero&&u.entityHandle==f.localHandle)self=&u;
 if(!self||strcmp(self->name,"npc_dota_hero_huskar")||self->illusion)return Block("Only your own Huskar.");
 if(contextHero&&(contextHero!=f.localHandle||contextRules!=f.rules)){contextHero=f.localHandle;contextRules=f.rules;machine.Reset();cfg::armletConfirmed=false;return Block("Hero / match changed. Confirm native setup again.");}
 contextHero=f.localHandle;contextRules=f.rules;
 uint32_t assigned=0;if(!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned)||assigned!=f.localHandle)return Block("Assigned hero handle changed / unavailable.");
 if(!Selected(f.localHandle))return Block("Select only your Huskar; selection unavailable otherwise.");
 if(!self->inventoryRead||!self->buffsRead)return Block("Inventory / modifier list unverified.");
 if(armletcore::Dangerous(self->buffs))return Block("Recognized damage-over-time / danger modifier.");
 uint64_t state=0;if(!mem::Read(self->addr+0x12B8,state)||(state&(killcore::CannotCast|killcore::Bit(1))))return Block("State unknown / item use restricted.");
 for(int i=0;i<self->abilN&&i<16;++i)if(self->abil[i].phase)return Block("Spell phase; no toggle.");
 const ItemInfo* item=nullptr;
 for(int i=0;i<self->itemN&&i<27;++i)if(!strcmp(self->items[i].icon,"armlet")&&self->items[i].slot==cfg::armletSlot)item=&self->items[i];
 if(!item||item->slot<0||item->slot>5||!item->instanceHandle||!mem::ValidPtr(item->addr))return Block("Armlet not in the configured active inventory slot.");
 uintptr_t identity=0,instance=0;uint32_t serial=0;
 if(!mem::Read(item->addr+off::instEntity,identity)||!mem::ValidPtr(identity)||!mem::Read(identity,instance)||instance!=item->addr||!mem::Read(identity+off::idHandleFld,serial)||serial!=item->instanceHandle)return Block("Item identity / serial no longer matches.");
 // Re-read RAW cooldown/toggle. HUD cooldown direction heuristics never drive input.
 uint8_t on=2,active=2,phase=2,frozen=2,indefinite=2;float cd=-1,muted=-1;
 if(!mem::Read(item->addr+0x62D,on)||on>1||!mem::Read(item->addr+0x638,cd)||!std::isfinite(cd)||cd<0||cd>3600||
    !mem::Read(item->addr+0x619,active)||active>1||!mem::Read(item->addr+0x634,phase)||phase>1||
    !mem::Read(item->addr+0x630,muted)||!std::isfinite(muted)||muted<0||!mem::Read(item->addr+0x654,indefinite)||indefinite>1||!mem::Read(item->addr+0x655,frozen)||frozen>1)return Block("Raw item toggle/readiness data unavailable.");
 // Active channel on any observed local spell prevents toggling, even if phase is false.
 uint32_t abilityHandle=0;
 if(!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,abilityHandle))return Block("Active ability state unavailable.");
 if(abilityHandle&&abilityHandle!=0xFFFFFFFFu&&abilityHandle!=item->instanceHandle)return Block("Active ability / possible channel.");
 bool ready=cd<=.01f&&active==1&&phase==0&&muted==0&&indefinite==0&&frozen==0;
 armletcore::Input in;in.allowed=true;in.on=on!=0;in.ready=ready;in.now=f.now;in.hp=self->hp;in.threshold=cfg::armletThreshold;in.hero=f.localHandle;in.item=item->instanceHandle;
 auto action=machine.Tick(in);
 using P=armletcore::Phase;
 switch(machine.State()){
 case P::Idle:status=ready?"Watching HP threshold; not an incoming-hit predictor.":"Item not ready; no input.";break;
 case P::WaitOff:status="OFF requested; waiting for observed OFF state.";break;
 case P::WaitOn:status="ON requested; waiting for observed ON state.";break;
 case P::Recover:status="ON observed; checking HP recovery / cooldown.";break;
 default:status="Cycle stopped. Armlet may be OFF; check manually and reset.";break;
 }
 if(action==armletcore::Action::None)return machine.Pending();
 if(GetForegroundWindow()!=hwnd||cfg::menuOpen||!cfg::armletAuto){machine.Failed();return Block("Focus changed");}
 INPUT inputs[2]{};inputs[0].type=inputs[1].type=INPUT_KEYBOARD;inputs[0].ki.wVk=inputs[1].ki.wVk=(WORD)key;inputs[1].ki.dwFlags=KEYEVENTF_KEYUP;
 UINT sent=SendInput(2,inputs,sizeof(INPUT));if(sent==1)SendInput(1,&inputs[1],sizeof(INPUT));if(sent!=2)machine.Failed();
 return true;
}
}
