#pragma once
#include "draft_lane_planner.h"
#include "draft_live.h"
#include "imgui.h"
#include <string>
#include <cstdio>
namespace draftadvisor {
inline void DrawUI(bool english,std::array<int,4>& manual){
 auto T=[&](const char* ru,const char* en){return english?en:ru;};
 static int manualTeam=2,requested=-1;static std::array<int,4> locks{{-1,-1,-1,-1}};
 ImGui::Checkbox(T("Автоматически читать союзные пики (экспериментально)","Auto-read ally picks (experimental)"),&automatic);
 if(ImGui::Button(T("Копировать диагностику пика","Copy draft diagnostics"))){char report[1536];snprintf(report,sizeof(report),"ESP17 Draft\nstage=%d auto=%d ready=%d source=%d team=%d localPlayer=%d controller=0x%llX resource=0x%llX layouts=%d,%d readMask=%d\nteamHeader=%llX,%llX,%llX playersHeader=%llX,%llX,%llX\nallies=%d,%d,%d,%d\noffsetsLiveVerified=0\n",probe.stage,automatic,live.ok,live.source,live.team,probe.localPlayer,(unsigned long long)probe.controller,(unsigned long long)probe.resource,live.playersLayout,live.teamsLayout,probe.readMask,(unsigned long long)probe.teamWord0,(unsigned long long)probe.teamWord8,(unsigned long long)probe.teamWord16,(unsigned long long)probe.playersWord0,(unsigned long long)probe.playersWord8,(unsigned long long)probe.playersWord16,live.allies[0],live.allies[1],live.allies[2],live.allies[3]);ImGui::SetClipboardText(report);}
 int own=automatic&&live.ok?live.ownHero:-1;
 auto picks=manual;int team=manualTeam;std::vector<int> blocked;
 if(automatic){
  if(!live.ok){ImGui::TextWrapped("%s",T("Авто-данных пока нет. Ожидание PlayerResource/локального игрока. Можно выключить авто и ввести пики вручную.","No automatic roster yet. Waiting for PlayerResource/local player. Disable auto to enter picks manually."));return;}
  picks=live.allies;team=live.team;blocked=live.excluded;
  ImGui::TextWrapped("%s",live.source==2?T("Источник: PlayerResource, два стабильных чтения. Смещения из статического дампа; live-версия не подтверждена.","Source: PlayerResource, two stable reads. Static-dump offsets; current live version not verified."):T("Источник: герои текущего матча. Это НЕ чтение пиков до появления героев.","Source: current-match hero entities. This is NOT pre-spawn draft reading."));
 }else{const char* teams[]={"Radiant","Dire"};int selected=manualTeam-2;ImGui::SetNextItemWidth(150);if(ImGui::Combo(T("Команда","Team"),&selected,teams,2))manualTeam=selected+2;team=manualTeam;}
 const char* positionsRu[]={"Предложить роль","1: Керри","2: Мидер","3: Оффлейнер","4: Поддержка","5: Саппорт"};
 const char* positionsEn[]={"Suggest position","1: Carry","2: Mid","3: Offlane","4: Support","5: Hard support"};
 auto positions=english?positionsEn:positionsRu;
 for(int slot=0;slot<4;++slot){ImGui::PushID(2300+slot);char label[32];snprintf(label,sizeof(label),"%s %d",T("Союзник","Ally"),slot+1);
  if(automatic){ImGui::Text("%s: %s",label,picks[slot]>=0?heroes[picks[slot]].label:T("Ещё не выбран","Not picked"));}
  else{ImGui::SetNextItemWidth(240);if(ImGui::BeginCombo(label,manual[slot]>=0?heroes[manual[slot]].label:T("Не выбран","Unset"))){
   if(ImGui::Selectable(T("Не выбран","Unset"),manual[slot]<0))manual[slot]=-1;
   for(int i=0;i<count;++i){bool used=false;for(int j=0;j<4;++j)if(j!=slot&&manual[j]==i)used=true;
    if(!used&&ImGui::Selectable(heroes[i].label,manual[slot]==i))manual[slot]=i;}ImGui::EndCombo();}picks=manual;}
  if(picks[slot]>=0){int role=locks[slot]+1;ImGui::SetNextItemWidth(230);if(ImGui::Combo(T("Роль союзника","Ally position"),&role,positions,6))locks[slot]=role-1;}
  ImGui::PopID();
 }
 int pos=requested+1;ImGui::SetNextItemWidth(240);if(ImGui::Combo(T("Моя роль","My position"),&pos,positions,6))requested=pos-1;
 int filled=0;for(int i:picks)if(i>=0)++filled;
 if(!filled){ImGui::TextWrapped("%s",T("Нужен хотя бы один выбранный союзник.","At least one ally pick is needed."));return;}
 // Cache deterministic planning; don't enumerate every role assignment each rendered frame.
 static std::array<int,4> priorPicks{{-2,-2,-2,-2}},priorLocks{{-2,-2,-2,-2}};static std::vector<int> priorBlocked;static int priorRequested=-2,priorOwn=-2;static std::vector<LaneSuggestion> suggestions;
 if(picks!=priorPicks||locks!=priorLocks||blocked!=priorBlocked||requested!=priorRequested||own!=priorOwn){suggestions=RecommendLanes(picks,locks,requested,blocked,own);priorOwn=own;priorPicks=picks;priorLocks=locks;priorBlocked=blocked;priorRequested=requested;}
 if(own>=0)ImGui::TextWrapped(T("Твой герой уже выбран: %s. План ниже относится к нему.","Your hero is already picked: %s. The plan below is for this hero."),heroes[own].label);
 ImGui::SeparatorText(T("Герой / линия / напарник","Hero / lane / partner"));
 if(suggestions.empty())ImGui::TextWrapped("%s",T("Нет подходящего распределения. Проверь роли союзников: нельзя занимать одну позицию дважды.","No valid assignment. Check ally role locks: positions cannot be duplicated."));
 const char* roleShort[]={"Carry","Mid","Offlane","Support","Hard support"};
 for(int i=0;i<3&&i<(int)suggestions.size();++i){const auto& v=suggestions[i];
  ImGui::Text("%d. %s - %s (%s)",i+1,heroes[v.hero].label,LaneName(team,v.position,english),roleShort[v.position]);
  if(v.partner>=0){ImGui::TextWrapped(T("Встать с %s. Дополняет линию: контроль/урон и совместимость ролей.","Lane with %s. Complementary roles and lane control/damage."),heroes[picks[v.partner]].label);}
  else ImGui::TextWrapped("%s",v.position==1?T("Мид: план для одиночной линии.","Mid: solo-lane plan."):T("Напарник на этой линии ещё не выбран.","A partner for this lane is not picked yet."));
 }
 ImGui::TextWrapped("%s",T("Если роли не заданы, распределение союзников предполагается по весам OpenHyperAI. Это совет, не реальные планы игроков и не гарантия лучшего пика. Линии меняются для Radiant/Dire; баны и матчапы не моделируются. Герой сам не выбирается.","Unset ally positions are inferred from OpenHyperAI weights. Advice, not actual player plans or a best-pick guarantee. Lanes mirror for Radiant/Dire; bans/matchups are not modeled. No automatic hero selection."));
}
}
