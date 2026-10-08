#include "skin_changer.h"
#include "imgui.h"
namespace skins {
void DrawSettings(bool english,float scale){
 (void)scale;auto T=[&](const char* ru,const char* en){return english?en:ru;};auto status=GetStatus();
 ImGui::TextWrapped("%s",T("Включение автоматически запрашивает настоящий кэш и добавляет отсутствующую косметику из каталога в локальный инвентарь. Выбирай и надевай предметы в родном арсенале Dota, а не в этом меню.","Enabling automatically requests your genuine cache and projects missing catalog cosmetics into the local inventory. Select and equip items in Dota's native armory, not in this menu."));
 ImGui::Separator();
 ImGui::TextDisabled("%s: %zu",T("Косметических определений в каталоге","Cosmetic definitions in catalog"),status.definitions);
 const char* state=!status.connected?T("Steam GC не подключён","Steam GC disconnected"):!status.enabled?T("Локальная проекция выключена","Local projection disabled"):!status.cacheReady?T("Ожидается настоящий кэш инвентаря","Waiting for genuine inventory cache"):T("Кэш получен; отображение в родном арсенале требует проверки в Dota","Cache captured; native armory display still needs live Dota verification");
 ImGui::TextWrapped("%s",state);ImGui::TextWrapped("%s",status.message.c_str());
 ImGui::BeginDisabled(!status.connected||!status.enabled);
 if(ImGui::Button(T("Повторить запрос при ошибке","Retry cache request on error")))Refresh();
 ImGui::EndDisabled();
 ImGui::TextWrapped("%s",T("При выключении возвращается настоящий локальный инвентарь. Предметы на аккаунт не добавляются; продажа и обмен не разблокируются.","Disabling restores your genuine local inventory. No items are added to your account; trading and selling are not unlocked."));
}
}
