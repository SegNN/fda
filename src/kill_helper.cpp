#include "kill_helper.h"
#include "local_visibility.h"
#include "imgui.h"
#include <algorithm>
namespace clientui {bool English();}
namespace killhelper {
Report Build(const Frame& f){
 Report out;if(!f.ok||f.observedOnly||!f.localAlive||f.localTeam<2||f.localTeam>3||!f.localHero||!f.queryUnit)return out;
 const FrameUnit *self=nullptr,*enemy=nullptr;
 for(const auto& u:f.units){if(u.kind!=UnitKind::Hero)continue;if(u.addr==f.localHero)self=&u;
  if(u.addr==f.queryUnit&&u.team!=f.localTeam&&(u.team==2||u.team==3)&&u.alive&&!u.illusion&&u.hp>0&&localvisibility::Get(f,u).visible&&std::isfinite(u.invis)&&u.invis<.01f)enemy=&u;}
 if(!self||!enemy)return out;
 out.target=true;out.hp=enemy->hp;snprintf(out.name,sizeof(out.name),"%s",enemy->nick);
 uint64_t own=0,target=0;bool ownRead=mem::Read(self->addr+0x12B8,own),targetRead=mem::Read(enemy->addr+0x12B8,target);
 float magic=0,armor=0;bool magicRead=mem::Read(enemy->addr+0x1630,magic),armorRead=mem::Read(enemy->addr+0x162C,armor);
 // Use current positions, not stale cached dist. Cast hull/range bonuses remain unmodeled.
 float dx=self->pos.x-enemy->pos.x,dy=self->pos.y-enemy->pos.y,dz=self->pos.z-enemy->pos.z;
 float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
 bool phase=false;for(int i=0;i<self->abilN&&i<16;++i)if(self->abil[i].phase)phase=true;
 for(int i=0;i<self->abilN&&i<16;++i){const auto& a=self->abil[i];Row row;row.level=a.level;
  auto* m=killhelpercore::Find(self->name,a.icon);snprintf(row.label,sizeof(row.label),"%s",m?m->label:a.icon);
  float raw=0,range=0;int type=m?m->type:cfg::ksDamageType;
  bool values=killhelpercore::Values(m,a.level,raw,range);
  // Unsupported skills may use the separately configured one-spell manual profile.
  if(!m&&a.level>0&&cfg::ksHeroName[0]&&!strcmp(self->name,cfg::ksHeroName)&&a.slot==cfg::ksAbilitySlot&&cfg::ksDamage>0&&cfg::ksRange>0){raw=cfg::ksDamage;range=cfg::ksRange;values=true;row.manual=true;}
  row.modeled=values;
  if(values){killcore::Input x;
   x.visible=true;x.alive=true;x.hp=enemy->hp;x.mana=f.mana;x.cost=a.mana;x.type=type;x.distance=distance;x.range=range;x.rawDamage=raw;x.margin=cfg::helperMargin;
   x.buffsRead=self->buffsRead&&enemy->buffsRead;x.stateRead=ownRead&&targetRead;x.localState=own;x.targetState=target;
   x.resistance=type==0?magic:armor;x.resistanceRead=type==2||(type==0?magicRead:armorRead);
   x.protectedTarget=(enemy->buffsRead&&killcore::Protected(enemy->buffs));x.ready=a.automationReady&&!phase;
   row.result=killhelpercore::Evaluate(x);
  }
  out.rows.push_back(row);
 }
 std::stable_sort(out.rows.begin(),out.rows.end(),[](const Row& a,const Row& b){
  auto rank=[](const Row& r){if(!r.modeled)return 20;using S=killhelpercore::Status;switch(r.result.status){case S::Enough:return 0;case S::Unknown:return 1;case S::Short:return 2;case S::Mana:return 3;case S::Range:return 4;case S::NotReady:return 5;default:return 6;}};
  return rank(a)<rank(b);
 });
 return out;
}
static const char* Status(killhelpercore::Status s,bool en){
 using S=killhelpercore::Status;switch(s){
 case S::Blocked:return en?"Protection / cast restriction":"Защита / запрет применения";
 case S::NotReady:return en?"Not ready / data unavailable":"Не готов / данные недоступны";
 case S::Mana:return en?"Not enough mana":"Не хватает маны";
 case S::Range:return en?"Beyond BASE range":"Вне БАЗОВОЙ дальности";
 case S::Short:return en?"Below HP + margin":"Меньше HP + запас";
 case S::Enough:return en?"BASE enough; hit not guaranteed":"По БАЗЕ хватает; попадание не гарантировано";
 default:return en?"Protection / stats unverified":"Защита / характеристики не проверены";
 }}
void Draw(const Frame& f){
 if(!cfg::showKillHelper||cfg::menuOpen)return;auto report=Build(f);if(!report.target)return;
 bool en=clientui::English();auto display=ImGui::GetIO().DisplaySize;
 float width=std::min(400.f,display.x-32.f),height=std::min(360.f,display.y-180.f);if(width<180||height<120)return;
 // Bottom right, distinct from effects at top left and map notifications at top right.
 ImGui::SetNextWindowPos(ImVec2(display.x-16.f,display.y-100.f),ImGuiCond_Always,ImVec2(1,1));
 const char* warning=en?"BASE impact snapshot. No talents, facets, upgrades, amplification, shields or flight prediction.":"БАЗА одного попадания. Без талантов, аспектов, улучшений, усиления, барьеров и прогноза полёта.";
 float inner=width-2*ImGui::GetStyle().WindowPadding.x;
 float needed=2*ImGui::GetTextLineHeightWithSpacing()+ImGui::CalcTextSize(warning,nullptr,false,inner).y+16+2*ImGui::GetStyle().WindowPadding.y;
 for(const auto& row:report.rows){if(cfg::helperModeledOnly&&(!row.modeled||row.level<=0))continue;
  char title[160];snprintf(title,sizeof(title),"%s | Lv %d%s",row.label,row.level,row.manual?(en?" (manual)":" (ручной)"):"");
  const char* status=row.modeled?Status(row.result.status,en):(row.level<=0?(en?"Not learned":"Не изучена"):(en?"No damage model":"Нет модели урона"));
  needed+=ImGui::CalcTextSize(title,nullptr,false,inner).y+ImGui::CalcTextSize(status,nullptr,false,inner).y+2*ImGui::GetStyle().ItemSpacing.y+8;
  if(row.modeled&&row.result.numeric)needed+=ImGui::GetTextLineHeightWithSpacing();
 }
 height=std::min(height,std::max(140.f,needed+ImGui::GetTextLineHeightWithSpacing()));
 ImGui::SetNextWindowBgAlpha(1.f);
 ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
 if(ImGui::Begin("##kill-helper",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoInputs)){
  ImGui::TextUnformatted(en?"Kill helper — estimates only":"Kill helper — только оценка");
  ImGui::TextDisabled(en?"Hover: %s | HP %d | margin %.0f":"Под курсором: %s | HP %d | запас %.0f",report.name,report.hp,cfg::helperMargin);
  ImGui::PushTextWrapPos(0);ImGui::TextDisabled("%s",warning);ImGui::PopTextWrapPos();ImGui::Separator();
  // Noninteractive HUD cannot scroll. Fit whole rows, never silently clip the deciding status.
  int shown=0,total=0;for(const auto& row:report.rows)if(!cfg::helperModeledOnly||(row.modeled&&row.level>0))++total;float footer=ImGui::GetTextLineHeightWithSpacing();
  for(const auto& row:report.rows){
   if(cfg::helperModeledOnly&&(!row.modeled||row.level<=0))continue;
   char title[160];snprintf(title,sizeof(title),"%s | Lv %d%s",row.label,row.level,row.manual?(en?" (manual)":" (ручной)"):"");
   const char* status=row.modeled?Status(row.result.status,en):(row.level<=0?(en?"Not learned":"Не изучена"):(en?"No damage model":"Нет модели урона"));
   float wrap=ImGui::GetContentRegionAvail().x;
   float needed=ImGui::CalcTextSize(title,nullptr,false,wrap).y+ImGui::CalcTextSize(status,nullptr,false,wrap).y+2*ImGui::GetStyle().ItemSpacing.y+8;
   if(row.modeled&&row.result.numeric)needed+=ImGui::GetTextLineHeightWithSpacing();
   if(ImGui::GetContentRegionAvail().y<needed+footer)break;
   ImGui::PushTextWrapPos(0);ImGui::TextUnformatted(title);
   if(row.modeled&&row.result.numeric)ImGui::Text(en?"Damage ~%.0f | damage - HP %+.0f":"Урон ~%.0f | урон - HP %+.0f",row.result.damage,row.result.delta);
   auto color=row.result.status==killhelpercore::Status::Enough?ImVec4(1.f,.76f,.28f,1):ImVec4(.64f,.67f,.72f,1);
   ImGui::TextColored(color,"%s",status);ImGui::PopTextWrapPos();ImGui::Separator();++shown;
  }
  if(total>shown)ImGui::TextDisabled(en?"+ %d skills outside panel":"+ %d способностей не поместилось",total-shown);
  else if(!total){ImGui::PushTextWrapPos(0);ImGui::TextDisabled("%s",en?"No learned modeled skills; check helper settings":"Нет изученных с моделью; проверьте настройки хелпера");ImGui::PopTextWrapPos();}
 }
 ImGui::End();
}
}
