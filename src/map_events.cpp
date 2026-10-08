#include "map_events.h"
#include "map_event_tracker.h"
#include "imgui.h"
#include <deque>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace clientui { bool English(); }
namespace mapevents {
static mapeventcore::Tracker tracker;
struct Toast { double until; float gameTime; mapeventcore::Event event; };
static std::deque<Toast> toasts;
static const char* RuneName(int type,bool en) {
    static const char* names[]={"Усиление урона","Ускорение","Иллюзии","Невидимость","Регенерация","Богатство","Волшебство","Вода","Мудрость","Щит"};
    static const char* english[]={"Damage amplification","Haste","Illusion","Invisibility","Regeneration","Bounty","Arcane","Water","Wisdom","Shield"};
    return type>=0&&type<10?(en?english[type]:names[type]):(en?"Rune":"Руна");
}
void UpdateAndDraw(const Frame& f) {
    using namespace mapeventcore;
    Settings settings{cfg::notifyRunes,cfg::notifyRuneSoon,cfg::notifyWards,cfg::notifyRoshan,cfg::notifyLead};
    std::vector<Observation> observations;
    if(f.ok)for(const auto& u:f.units) {
        Observation o; o.handle=u.entityHandle;o.x=u.pos.x;o.y=u.pos.y;o.alive=u.alive;
        if(u.kind==UnitKind::Rune && u.runeRead) {o.kind=Kind::Rune;o.type=u.runeType;o.stamp=0;}
        else if(u.kind==UnitKind::RuneSpawner && u.runeRead) {o.kind=Kind::Spawner;o.type=u.runeType;o.nextType=u.nextRuneType;o.stamp=u.runeLastSpawn;o.next=u.runeNextSpawn;}
        else if(u.kind==UnitKind::Ward && u.alive && u.team!=f.localTeam &&
                u.teamVisibilityRead && f.localTeam>=2 && f.localTeam<=3 &&
                (u.teamVisibilityMask&(1u<<f.localTeam)))o.kind=Kind::EnemyWard;
        else if(u.kind==UnitKind::Roshan)o.kind=Kind::Roshan;
        else continue;
        observations.push_back(o);
    }
    static uintptr_t priorRules=0;static float priorStart=-1,priorNow=-1;
    if(!f.ok || f.rules!=priorRules || f.gameStart!=priorStart || f.now<priorNow-.25f || f.now-priorNow>5.f)toasts.clear();
    priorRules=f.rules;priorStart=f.gameStart;priorNow=f.now;
    auto events=tracker.Update(f.ok,(uint64_t)f.rules,f.gameStart,f.now,observations,settings);
    double now=ImGui::GetTime();
    if(!f.ok)toasts.clear();
    for(const auto& e:events){toasts.push_back({now+6.,f.now,e});while(toasts.size()>4)toasts.pop_front();}
    for(auto it=toasts.begin();it!=toasts.end();) {
        using EK=EventKind; auto k=it->event.kind;
        bool enabled=(k==EK::RuneSoon?settings.soon:(k==EK::WardDiscovered?settings.wards:
                     (k==EK::RoshanAlive||k==EK::RoshanDead?settings.roshan:settings.runes)));
        if(it->until<=now||!enabled||(k==EK::RuneSoon&&it->event.seconds-(f.now-it->gameTime)<=0))it=toasts.erase(it);else ++it;
    }
    if(toasts.empty())return;
    bool en=clientui::English();
    auto display=ImGui::GetIO().DisplaySize;
    float width=std::min(340.f,std::max(100.f,display.x-32.f));
    ImGui::SetNextWindowPos(ImVec2(display.x-16,80),ImGuiCond_Always,ImVec2(1,0));
    ImGui::SetNextWindowSize(ImVec2(width,0));
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,5);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14,12));
    if(ImGui::Begin("##map-events",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
            ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav)) {
        for(size_t i=0;i<toasts.size();++i) {
            const auto& e=toasts[i].event;char text[192];const char* evidence=en?"Available to the client":"Доступно клиенту";
            switch(e.kind) {
            case EventKind::RuneDiscovered:snprintf(text,sizeof(text),en?"Rune discovered: %s":"Обнаружена руна: %s",RuneName(e.type,en));break;
            case EventKind::RuneSpawnObserved:snprintf(text,sizeof(text),en?"Rune spawn: client data":"Спавн руны: данные клиента");evidence=en?"Spawner timestamp changed":"Изменилось время в спавнере";break;
            case EventKind::RuneSoon:snprintf(text,sizeof(text),en?"%s: due in ~%.0f s":"%s: появление через ~%.0f с",RuneName(e.type,en),std::max(0.f,e.seconds-(f.now-toasts[i].gameTime)));evidence=en?"Reminder, not a confirmed spawn":"Напоминание, не подтверждённый спавн";break;
            case EventKind::WardDiscovered:snprintf(text,sizeof(text),en?"Enemy ward discovered":"Обнаружен вражеский вард");break;
            case EventKind::RoshanAlive:snprintf(text,sizeof(text),en?"Roshan alive again: client data":"Рошан снова жив: данные клиента");evidence=en?"Confirmed by readable health":"Подтверждено прочитанными HP";break;
            case EventKind::RoshanDead:snprintf(text,sizeof(text),en?"Roshan died: client data":"Рошан погиб: данные клиента");evidence=en?"Readable health state changed":"Прочитано изменение HP";break;
            }
            ImGui::PushTextWrapPos(0);ImGui::TextUnformatted(text);
            ImGui::TextDisabled("%s",evidence);ImGui::TextDisabled("x: %.0f  y: %.0f",e.x,e.y);ImGui::PopTextWrapPos();
            if(i+1<toasts.size()) {ImGui::Spacing();ImGui::Separator();ImGui::Spacing();}
        }
    }
    ImGui::End();ImGui::PopStyleVar(2);
}
}
