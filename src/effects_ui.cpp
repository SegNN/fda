#include "effects_ui.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
namespace clientui {bool English();}
namespace effectsui {
static const char* Label(const char* n){
 if(!strcmp(n,"modifier_flask_healing"))return "Flask";
 if(!strcmp(n,"modifier_tango_heal"))return "Tango";
 if(!strcmp(n,"modifier_clarity_potion"))return "Clarity";
 return !strncmp(n,"modifier_",9)?n+9:n;
}
void Draw(const Frame& f){
 if(!cfg::showEffects||!f.ok)return;
 const FrameUnit* hero=nullptr;
 for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.addr==f.localHero)hero=&u;
 for(const auto& u:f.units)if(u.kind==UnitKind::Hero&&u.addr==f.queryUnit&&
   (u.team==f.localTeam||(u.teamVisibilityRead&&f.localTeam>=2&&f.localTeam<=3&&(u.teamVisibilityMask&(1u<<f.localTeam)))))hero=&u;
 if(!hero)return;
 bool en=clientui::English();auto display=ImGui::GetIO().DisplaySize;
 ImGui::SetNextWindowPos(ImVec2(16,80),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(std::min(320.f,display.x-32),0));
 ImGui::SetNextWindowSizeConstraints(ImVec2(160,0),ImVec2(std::min(320.f,display.x-32),std::max(100.f,display.y-112)));
 if(ImGui::Begin("##active-effects",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoInputs)){
  ImGui::TextUnformatted(en?"Active effects (client data)":"Активные эффекты (данные клиента)");
  ImGui::TextDisabled("%s",hero->nick);ImGui::Separator();
  ImGui::PushTextWrapPos(0);
  if(!hero->buffsRead)ImGui::TextUnformatted(en?"Modifier list unavailable. No effects inferred from inventory.":"Список модификаторов недоступен. Наличие предмета не считается активным эффектом.");
  else{
   int shown=0,total=0;float used=0,budget=std::max(40.f,display.y-210.f);
   for(const auto& b:hero->buffs){float rem=buffreader::Remaining(b,f.now);if(cfg::effectsTimedOnly&&b.duration<0)continue;if(rem==0&&b.duration>=0)continue;++total;
    if(shown>=12)continue;char line[192];
    if(rem>=0)snprintf(line,sizeof(line),"%s  %.1f s  [%d]",Label(b.name),rem,b.stacks);
    else snprintf(line,sizeof(line),"%s  %s  [%d]",Label(b.name),rem==-1?(en?"permanent":"постоянный"):(en?"time unknown":"время неизвестно"),b.stacks);
    float height=ImGui::CalcTextSize(line,nullptr,false,ImGui::GetContentRegionAvail().x).y+ImGui::GetStyle().ItemSpacing.y;
    if(used+height>budget)continue;used+=height;++shown;ImGui::TextUnformatted(line);
   }
   if(!total)ImGui::TextDisabled("%s",en?"No readable effects for this filter":"Нет прочитанных эффектов по фильтру");
   if(total>shown)ImGui::TextDisabled(en?"+ %d effects":"+ %d эффектов",total-shown);
  }
  ImGui::PopTextWrapPos();
 }
 ImGui::End();
}
}
