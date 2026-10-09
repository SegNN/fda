#include "armlet.h"
#include "armlet_core.h"
#include "damage_probe.h"
namespace armlet {
static armletcore::Machine machine;
static const char* status="OFF / no input";
static const char* lastClosedStatus="Not sampled with menu closed.";
const char* LastClosedStatus(){return lastClosedStatus;}
struct CaptureClosedStatus {~CaptureClosedStatus();};
static char lastDangerName[128]={};static float dangerCheckedAt=-1;static uint32_t dangerHero=0;
const char* DangerDiagnostics(){static char report[256];snprintf(report,sizeof(report),"danger: lastCheckedAt=%g hero=0x%X matched=%s\n",dangerCheckedAt,dangerHero,lastDangerName[0]?lastDangerName:"none");return report;}
static char firstFault[512]={};static float tickTime=0;static int tickHP=-1,observedOn=-1,phaseBefore=0;static uint32_t tickHero=0;
const char* CycleDiagnostics(){static char text[768];snprintf(text,sizeof(text),"cycle: phase=%d firstFault=%s\n%s",int(machine.State()),machine.FaultReason(),machine.State()==armletcore::Phase::Fault?firstFault:"");return text;}
struct CaptureFault {~CaptureFault(){if(machine.State()==armletcore::Phase::Fault&&(phaseBefore!=int(armletcore::Phase::Fault)||!firstFault[0]))snprintf(firstFault,sizeof(firstFault),"faultContext: time=%g hero=0x%X hp=%d phaseBefore=%d observedOn=%d\n",tickTime,tickHero,tickHP,phaseBefore,observedOn);}};
// Diagnostic observations only: they do not permit/queue any native input.
static constexpr uint64_t ItemRestrictedMask=killcore::Bit(4)|killcore::Bit(5)|killcore::Bit(6)|killcore::Bit(8)|killcore::Bit(11)|killcore::Bit(20)|killcore::Bit(33);
static uint64_t tickUnitState=0;static bool tickStateRead=false,tickRawRead=false;
static uint32_t tickItem=0;static int tickReady=-1,tickAction=0,tickEffect=-1;
static float tickCooldown=-1;static int tickActive=-1,tickItemPhase=-1,tickFrozen=-1,tickIndefinite=-1;static float tickMuted=-1;
static unsigned inputCalls=0,requestedOff=0,requestedOn=0,deliveredPairs=0,deliveryFailures=0,resetRequests=0;
static float lastInputAt=-1;static int lastSent=-1,lastInputAction=0;
static char lastClosedObservation[1024]="closedTick: not sampled with menu closed.\n";
static char events[16][512]={};static unsigned eventCount=0;
static const char* previousTraceStatus=nullptr;static int previousTracePhase=-1,previousGate=-1;static uint32_t previousTraceHero=0;
CaptureClosedStatus::~CaptureClosedStatus(){
 if(cfg::menuOpen)return;
 lastClosedStatus=status;int phase=int(machine.State());int hpGate=tickHP>0&&std::isfinite(cfg::armletThreshold)?int(tickHP<=cfg::armletThreshold):-1;
 snprintf(lastClosedObservation,sizeof(lastClosedObservation),"closedTick: at=%g hero=0x%X hp=%d threshold=%g hpLEthreshold=%d phase=%d on=%d ready=%d effectOn=%d item=0x%X\nclosedGate: stateRead=%d state=0x%llX blockedMask=0x%llX rawRead=%d cd=%g active=%d itemPhase=%d muted=%g indefinite=%d frozen=%d status=%s\n",tickTime,tickHero,tickHP,cfg::armletThreshold,hpGate,phase,observedOn,tickReady,tickEffect,tickItem,tickStateRead,(unsigned long long)tickUnitState,(unsigned long long)(tickUnitState&ItemRestrictedMask),tickRawRead,tickCooldown,tickActive,tickItemPhase,tickMuted,tickIndefinite,tickFrozen,status);
 if(tickAction||!previousTraceStatus||strcmp(previousTraceStatus,status)||phase!=previousTracePhase||hpGate!=previousGate||tickHero!=previousTraceHero){
  snprintf(events[eventCount%16],sizeof(events[0]),"event[%u]: at=%g hero=0x%X hp=%d threshold=%g hpLEthreshold=%d phase=%d on=%d ready=%d action=%d inputCalls=%u deliveredPairs=%u failures=%u status=%s fault=%s\n",eventCount,tickTime,tickHero,tickHP,cfg::armletThreshold,hpGate,phase,observedOn,tickReady,tickAction,inputCalls,deliveredPairs,deliveryFailures,status,machine.FaultReason());++eventCount;
 }
 previousTraceStatus=status;previousTracePhase=phase;previousGate=hpGate;previousTraceHero=tickHero;
}
const char* TraceDiagnostics(){
 static char report[10240];size_t used=size_t(snprintf(report,sizeof(report),"armletInput: inputCalls=%u requestedOff=%u requestedOn=%u deliveredPairs=%u deliveryFailures=%u resets=%u lastAt=%g lastAction=%d lastSent=%d\nNOTE deliveredPairs means SendInput accepted key events, NOT that Dota toggled Armlet. on=-1/ready=-1 means gate not reached; no inferred value.\n%s",inputCalls,requestedOff,requestedOn,deliveredPairs,deliveryFailures,resetRequests,lastInputAt,lastInputAction,lastSent,lastClosedObservation));
 if(firstFault[0]&&machine.State()!=armletcore::Phase::Fault)used+=size_t(snprintf(report+used,sizeof(report)-used,"historicalOnly_%s",firstFault));
 unsigned first=eventCount>16?eventCount-16:0;
 for(unsigned i=first;i<eventCount&&used<sizeof(report)-1;++i){int n=snprintf(report+used,sizeof(report)-used,"%s",events[i%16]);if(n>0)used+=std::min(size_t(n),sizeof(report)-used-1);}
 return report;
}
static char damageReport[6144]="damageProbe: not sampled; diagnostics only, not a damage predictor.\n";
static float probeAt=-1000;static uint32_t probeHero=0;
const char* DamageDiagnostics(){return damageReport;}
struct DamageReader {
 uintptr_t localHero=0;
 bool Valid(uintptr_t p){return mem::ValidPtr(p);}
 bool Owner(uint32_t h){return game::EntityByHandle(h)==localHero;}
 template<class T>bool Read(uintptr_t a,T& v){return mem::Read(a,v);}
 const char* Class(uintptr_t vp){return rtti::ClassOf(game::g_sys.clientBase,vp);}
};
static void SampleDamage(const Frame& f){
 if(!cfg::armletAuto)return;
 if(!f.ok||f.observedOnly||!f.localHandle||!std::isfinite(f.now)){snprintf(damageReport,sizeof(damageReport),"damageProbe: local frame unavailable; no current sample.\n");return;}
 if(probeHero==f.localHandle&&f.now>=probeAt&&f.now-probeAt<.2f)return;
 probeAt=f.now;probeHero=f.localHandle;
 const FrameUnit* self=nullptr;for(const auto& u:f.units)if(u.addr==f.localHero&&u.entityHandle==f.localHandle)self=&u;
 if(!self||!self->buffsRead){snprintf(damageReport,sizeof(damageReport),"damageProbe: modifier list unverified; no current sample.\n");return;}
 size_t used=size_t(snprintf(damageReport,sizeof(damageReport),"damageProbe: at=%g hero=0x%X hp=%d candidates=%zu liveOffsetsVerified=0 predictionReady=0\nbaseBuffPreviousTickOffset=unknown baseBuffThinkIntervalOffset=unknown\n",f.now,f.localHandle,self->hp,damageprobe::CatalogSize));
 DamageReader reader{f.localHero};int shown=0,unsupported=0;
 for(const auto& b:self->buffs){if(shown>=6||used>=sizeof(damageReport)-512)break;auto sample=damageprobe::Read(reader,b,f.localHandle);if(!sample.identity){++unsupported;continue;}if(!sample.knownClass){++unsupported;continue;}
  used+=damageprobe::Format(damageReport+used,sizeof(damageReport)-used,b,sample);++shown;
 }
 if(used<sizeof(damageReport)-1)snprintf(damageReport+used,sizeof(damageReport)-used,"probeShown=%d omittedOrUnsupportedAtLeast=%d totalModifiers=%zu (bounded diagnostic snapshot)\n",shown,unsupported,self->buffs.size());
}
static uintptr_t contextRules=0;static uint32_t contextHero=0;
const char* Status(){return status;}
const char* StatusRu(){
 struct Message{const char* en;const char* ru;};
 for(auto m:{Message{"OFF","Выключено."},
 Message{"Confirm native key and risk after testing in demo.","Не включено подтверждение клавиши. Сначала проверьте её вручную в демо, затем подтвердите в меню."},
 Message{"Local player unresolved; no automatic Huskar selection.","Локальный игрок не определён (localFrame=0). Автонажатия заблокированы; нужен полный HUD6-отчёт диагностики."},
 Message{"Close the menu to run Armlet.","Закройте меню программы: с открытым меню автонажатия запрещены."},
 Message{"Dota must have focus.","Окно Dota должно быть активным."},
 Message{"Select only your Huskar; selection unavailable otherwise.","Выберите только своего Huskar. Группа юнитов или неподтверждённое выделение блокируют цикл."},
 Message{"Inventory / modifier list unverified.","Инвентарь или полный список модификаторов не подтверждены. Видимые иконки ESP этого не подтверждают."},
 Message{"Only your own Huskar.","Работает только на вашем Huskar, не на чужом герое или иллюзии."},
 Message{"Armlet not in the configured active inventory slot.","Armlet не найден в выбранном активном слоте 1–6. Проверьте слот в меню."},
 Message{"Damage timing unknown; new OFF blocked, ON completion allowed.","Время входящего урона неизвестно: новый OFF запрещён, завершение ON не отменяется из-за DoT."},
 Message{"HP above threshold; no toggle requested.","HP выше порога: переключение не запрашивалось."},
 Message{"Minimum cycle interval; no input.","Жду окончания минимального интервала между циклами."},
 Message{"Waiting for rearm; no input.","Жду повторного взведения цикла; нажатий нет."},
 Message{"Unit-state mask blocks item use.","Маска состояния героя блокирует использование предмета; см. blockedMask."},
 Message{"Local unit-state unreadable.","Состояние героя не удалось прочитать; ввод заблокирован."},
 Message{"Watching HP threshold; not an incoming-hit predictor.","Готов: жду HP ниже порога. Входящие удары НЕ предсказываются."},
 Message{"Item not ready; no input.","Предмет не готов: нажатий нет."},
 Message{"OFF requested; waiting for observed OFF state.","Запрошено выключение: жду подтверждения OFF."},
 Message{"ON requested; waiting for observed ON state.","Запрошено включение: жду подтверждения ON."},
 Message{"ON and active effect observed; settling before rearm.","ON и активный модификатор подтверждены; жду стабилизации перед новым циклом."},
 Message{"Cycle stopped. Armlet may be OFF; check manually and reset.","Цикл остановлен. Armlet может остаться выключенным: проверьте вручную, затем сбросьте цикл."},
 Message{"Native item key required; optional activation binding must not collide.","Нужна реальная клавиша слота предмета. Она не должна совпадать с биндом модуля, меню или выгрузки."}})if(!strcmp(status,m.en))return m.ru;
 return status; // Preserve the exact diagnostic when no translation exists.
}
bool Pending(){return machine.Pending();}
bool OwnsLane(const Frame& f){
 if(machine.Pending())return true;
 if(!cfg::armletAuto||!f.ok||f.observedOnly||!f.localHandle)return false;
 for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.addr==f.localHero&&u.entityHandle==f.localHandle&&!u.illusion&&!strcmp(u.name,"npc_dota_hero_huskar"))return true;
 return false;
}
void Reset(){++resetRequests;machine.Reset();firstFault[0]=0;status="Reset. Verify native Armlet key, slot and state manually.";}
static bool Block(const char* why){machine.Cancel(why);status=machine.State()==armletcore::Phase::Fault?"Cycle stopped. Armlet may be OFF; check manually and reset.":why;return false;}
static char selectionReport[1536]="selection: not sampled at the selection gate.";
const char* SelectionDiagnostics(){return selectionReport;}
static bool Selected(uint32_t hero){
 uintptr_t ctrl=game::g_sys.localCtrl;if(!mem::ValidPtr(ctrl)){snprintf(selectionReport,sizeof(selectionReport),"selection: controller unavailable");return false;}
 uint64_t raw[3]={};int rawMask=0;for(int i=0;i<3;++i)if(mem::Read(ctrl+0x9C8+8*i,raw[i]))rawMask|=1<<i;
 uint32_t query=0;uint8_t inQuery=2;bool queryRead=mem::Read(ctrl+off::CtrlHUD::m_hQueryUnit,query),inQueryRead=mem::Read(ctrl+0xA00,inQuery);
 snprintf(selectionReport,sizeof(selectionReport),"selection: expectedHandle=0x%X expectedIndex=%u rawMask=%d raw0=0x%llX raw8=0x%llX raw16=0x%llX queryRead=%d query=0x%X inQueryRead=%d inQuery=%u\n",hero,hero&off::handleMask,rawMask,(unsigned long long)raw[0],(unsigned long long)raw[1],(unsigned long long)raw[2],queryRead,query,inQueryRead,(unsigned)inQuery);
 struct Shape {int count,data;};const Shape shapes[]={{0,8},{16,0}};
 bool accepted=false;size_t used=strlen(selectionReport);
 for(auto shape:shapes){uintptr_t data=0,again=0;int n=-1,index=-1,check=-1,indexAgain=-1;
  bool countRead=mem::Read(ctrl+0x9C8+shape.count,n),dataRead=mem::Read(ctrl+0x9C8+shape.data,data);
  bool indexRead=n==1&&dataRead&&mem::ValidPtr(data)&&mem::Read(data,index);
  bool stable=countRead&&indexRead&&mem::Read(ctrl+0x9C8+shape.count,check)&&mem::Read(ctrl+0x9C8+shape.data,again)&&mem::Read(data,indexAgain)&&n==check&&data==again&&index==indexAgain;
  // Accept an exact index OR exact full handle. Never strip serial bits from a mismatched handle.
  bool match=stable&&(uint32_t(index)==hero||(index>=0&&uint32_t(index)==(hero&off::handleMask)));
  if(match)accepted=true;
  int len=snprintf(selectionReport+used,sizeof(selectionReport)-used,"candidate count+%d/data+%d: countRead=%d count=%d dataRead=%d data=0x%llX indexRead=%d value=0x%X stable=%d match=%d\n",shape.count,shape.data,countRead,n,dataRead,(unsigned long long)data,indexRead,uint32_t(index),stable,match);
  if(len>0)used+=std::min(size_t(len),sizeof(selectionReport)-used-1);
 }
 return accepted;
}
bool Tick(const Frame& f){
 SampleDamage(f);
 CaptureClosedStatus capture;
 CaptureFault faultCapture;tickTime=f.now;tickHero=f.localHandle;tickHP=-1;observedOn=-1;phaseBefore=int(machine.State());
 tickUnitState=0;tickStateRead=tickRawRead=false;tickItem=0;tickReady=tickEffect=-1;tickAction=0;tickCooldown=tickMuted=-1;tickActive=tickItemPhase=tickFrozen=tickIndefinite=-1;
 for(const auto& unit:f.units)if(unit.addr==f.localHero&&unit.entityHandle==f.localHandle)tickHP=unit.hp;
 if(!cfg::armletAuto)return Block("OFF");
 if(!cfg::armletConfirmed)return Block("Confirm native key and risk after testing in demo.");
 int key=cfg::armletKey,bind=binds::items[11].vk;
 if(!((key>='A'&&key<='Z')||(key>='0'&&key<='9'))||bind<0||bind>=256||(bind>0&&(key==bind||bind==cfg::menuKey||bind==cfg::unloadKey))||key==cfg::menuKey||key==cfg::unloadKey)return Block("Native item key required; optional activation binding must not collide.");
 if(!f.ok||f.observedOnly||!f.localHandle)return Block("Local player unresolved; no automatic Huskar selection.");
 if(cfg::menuOpen)return Block("Close the menu to run Armlet.");
 if(!f.localAlive||!std::isfinite(f.now))return Block("Local hero dead / clock unavailable.");
 uint8_t paused=2;if(!mem::ValidPtr(f.rules)||!mem::Read(f.rules+0x38,paused)||paused!=0)return Block("Pause state unknown / game paused.");
 HWND hwnd=GetForegroundWindow();DWORD pid=0;if(!hwnd||!IsWindowVisible(hwnd)||!GetWindowThreadProcessId(hwnd,&pid)||pid!=GetCurrentProcessId())return Block("Dota must have focus.");
 for(int vk:{VK_SHIFT,VK_CONTROL,VK_MENU,key})if(GetAsyncKeyState(vk)&0x8000)return Block("User key/modifier held.");
 const FrameUnit* self=nullptr;for(const auto& u:f.units)if(u.addr==f.localHero&&u.kind==UnitKind::Hero&&u.entityHandle==f.localHandle)self=&u;
 if(!self||strcmp(self->name,"npc_dota_hero_huskar")||self->illusion)return Block("Only your own Huskar.");
 if(contextHero&&(contextHero!=f.localHandle||contextRules!=f.rules)){contextHero=f.localHandle;contextRules=f.rules;machine.Reset();firstFault[0]=0;cfg::armletConfirmed=false;return Block("Hero / match changed. Confirm native setup again.");}
 contextHero=f.localHandle;contextRules=f.rules;
 uint32_t assigned=0;if(!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hAssignedHero,assigned)||assigned!=f.localHandle)return Block("Assigned hero handle changed / unavailable.");
 if(!Selected(f.localHandle))return Block("Select only your Huskar; selection unavailable otherwise.");
 if(!self->inventoryRead||!self->buffsRead)return Block("Inventory / modifier list unverified.");
 bool effectOn=false;for(const auto& buff:self->buffs)if(!strcmp(buff.name,"modifier_item_armlet_unholy_strength"))effectOn=true;
 tickEffect=effectOn?1:0;
 const char* danger=armletcore::DangerousName(self->buffs);
 dangerCheckedAt=f.now;dangerHero=f.localHandle;snprintf(lastDangerName,sizeof(lastDangerName),"%s",danger?danger:"");
 // Keep reading identity/readiness; risk blocks only new OFF, not completion of ON.
 tickStateRead=mem::Read(self->addr+off::NPC::m_nUnitState64,tickUnitState);
 if(!tickStateRead)return Block("Local unit-state unreadable.");
 if(tickUnitState&ItemRestrictedMask)return Block("Unit-state mask blocks item use.");
 for(int i=0;i<self->abilN&&i<16;++i)if(self->abil[i].phase)return Block("Spell phase; no toggle.");
 const ItemInfo* item=nullptr;
 for(int i=0;i<self->itemN&&i<27;++i)if(!strcmp(self->items[i].icon,"armlet")&&self->items[i].slot==cfg::armletSlot)item=&self->items[i];
 if(!item||item->slot<0||item->slot>5||!item->instanceHandle||!mem::ValidPtr(item->addr))return Block("Armlet not in the configured active inventory slot.");
 uintptr_t identity=0,instance=0;uint32_t serial=0;
 if(!mem::Read(item->addr+off::instEntity,identity)||!mem::ValidPtr(identity)||!mem::Read(identity,instance)||instance!=item->addr||!mem::Read(identity+off::idHandleFld,serial)||serial!=item->instanceHandle)return Block("Item identity / serial no longer matches.");
 tickItem=item->instanceHandle;
 // Re-read RAW cooldown/toggle. HUD cooldown direction heuristics never drive input.
 uint8_t on=2,active=2,phase=2,frozen=2,indefinite=2;float cd=-1,muted=-1;
 bool rawOK=mem::Read(item->addr+0x62D,on)&&on<=1&&mem::Read(item->addr+0x638,cd)&&std::isfinite(cd)&&cd>=0&&cd<=3600&&
    mem::Read(item->addr+0x619,active)&&active<=1&&mem::Read(item->addr+0x634,phase)&&phase<=1&&
    mem::Read(item->addr+0x630,muted)&&std::isfinite(muted)&&muted>=0&&mem::Read(item->addr+0x654,indefinite)&&indefinite<=1&&mem::Read(item->addr+0x655,frozen)&&frozen<=1;
 tickRawRead=rawOK;tickCooldown=cd;tickMuted=muted;tickActive=active;tickItemPhase=phase;tickIndefinite=indefinite;tickFrozen=frozen;
 if(!rawOK)return Block("Raw item toggle/readiness data unavailable.");
 // Active channel on any observed local spell prevents toggling, even if phase is false.
 uint32_t abilityHandle=0;
 if(!mem::Read(game::g_sys.localCtrl+off::Ctrl::m_hActiveAbility,abilityHandle))return Block("Active ability state unavailable.");
 if(abilityHandle&&abilityHandle!=0xFFFFFFFFu&&abilityHandle!=item->instanceHandle)return Block("Active ability / possible channel.");
 observedOn=int(on);
 bool ready=cd<=.01f&&active==1&&phase==0&&muted==0&&indefinite==0&&frozen==0;
 tickReady=ready?1:0;
 armletcore::Input in;in.allowed=true;in.allowTurnOff=danger==nullptr;in.effectRead=self->buffsRead;in.effectOn=effectOn;in.on=on!=0;in.ready=ready;in.now=f.now;in.hp=self->hp;in.threshold=cfg::armletThreshold;in.hero=f.localHandle;in.item=item->instanceHandle;
 auto action=machine.Tick(in);
 using P=armletcore::Phase;
 switch(machine.State()){
 case P::Idle:status=!ready?"Item not ready; no input.":(danger&&on?"Damage timing unknown; new OFF blocked, ON completion allowed.":(self->hp>cfg::armletThreshold?"HP above threshold; no toggle requested.":machine.CooldownRemaining(f.now)>0?"Minimum cycle interval; no input.":!machine.Armed()?"Waiting for rearm; no input.":"Watching HP threshold; not an incoming-hit predictor."));break;
 case P::WaitOff:status="OFF requested; waiting for observed OFF state.";break;
 case P::WaitOn:status="ON requested; waiting for observed ON state.";break;
 case P::Recover:status="ON and active effect observed; settling before rearm.";break;
 default:status="Cycle stopped. Armlet may be OFF; check manually and reset.";break;
 }
 if(action==armletcore::Action::None)return machine.Pending();
 if(GetForegroundWindow()!=hwnd||cfg::menuOpen||!cfg::armletAuto){machine.Failed("Focus/menu/enable changed before input");return Block("Focus changed");}
 INPUT inputs[2]{};inputs[0].type=inputs[1].type=INPUT_KEYBOARD;inputs[0].ki.wVk=inputs[1].ki.wVk=(WORD)key;inputs[1].ki.dwFlags=KEYEVENTF_KEYUP;
 tickAction=action==armletcore::Action::TurnOff?1:2;lastInputAction=tickAction;lastInputAt=f.now;++inputCalls;if(tickAction==1)++requestedOff;else ++requestedOn;
 UINT sent=SendInput(2,inputs,sizeof(INPUT));lastSent=int(sent);if(sent==2)++deliveredPairs;else ++deliveryFailures;if(sent==1)SendInput(1,&inputs[1],sizeof(INPUT));if(sent!=2){machine.Failed("SendInput did not deliver both key events");status="Cycle stopped. Armlet may be OFF; check manually and reset.";}
 return true;
}
}
