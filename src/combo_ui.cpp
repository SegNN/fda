#include "combos.h"
#include "imgui.h"
namespace combos {
void DrawSettings(int index,bool english){if(index<0||index>=count)return;InitializeSettings();auto& config=settings[index];const auto& p=profiles[index];
 auto T=[&](const char* ru,const char* en){return english?en:ru;};
 ImGui::Checkbox(T("Включить базовый прокаст","Enable basic combo"),&config.enabled);
 ImGui::TextWrapped("%s",T("Это базовая последовательность заклинаний, НЕ полный порт Umbrella. Без предметов, Blink, прогноза попаданий и special hero mechanics.","Basic spell sequence, NOT a complete Umbrella port. No items, Blink, hit prediction or special hero mechanics."));
 const char* choices[]={"Unset","Space","Q","W","E","R","D","F","Z","X","C","V","B","N","G","H","J","K","L","T","Y","U","I","O","P","1","2","3","4","5","6"};
 const int keys[]={0,VK_SPACE,'Q','W','E','R','D','F','Z','X','C','V','B','N','G','H','J','K','L','T','Y','U','I','O','P','1','2','3','4','5','6'};
 auto keyControl=[&](const char* label,int& key){int selected=0;for(int i=0;i<IM_ARRAYSIZE(keys);++i)if(keys[i]==key)selected=i;ImGui::SetNextItemWidth(130);if(ImGui::Combo(label,&selected,choices,IM_ARRAYSIZE(keys))){key=keys[selected];config.confirmed=false;}};
 keyControl(T("Бинд: удерживать","Hold binding"),config.holdKey);
 if(!config.holdKey&&ImGui::Button(T("Назначить Space для запуска","Set Space activation"))){config.holdKey=VK_SPACE;config.confirmed=false;}
 for(int i=0;i<p.count;++i){ImGui::PushID(i);ImGui::Text("%d. %s (<= %.0f)",i+1,p.steps[i].ability,p.steps[i].range);keyControl(T("Клавиша quickcast","Native quickcast key"),config.keys[i]);ImGui::PopID();}
 ImGui::Checkbox(T("Клавиши проверены в демо: quickcast на нажатие","Keys tested in demo: quickcast on keydown"),&config.confirmed);
 ImGui::TextWrapped("%s",T("Выбери только собственного героя. Наведи курсор на видимого врага, закрой меню и удерживай бинд. Одна последовательность на удержание; после конца/ошибки отпусти. Следующий спелл только после подтверждённого cooldown предыдущего. Не гарантия попадания или убийства.","Select only your own hero, aim cursor near a visible enemy, close menu and hold binding. One sequence per hold; release after completion/failure. Next spell requires previous spell cooldown confirmation. No hit/kill guarantee."));
 ImGui::Separator();ImGui::TextWrapped("%s",T("Запуск: закрыть меню  /  выбрать своего героя  /  курсор на врага в пределах указанной дальности  /  удерживать отдельный бинд. Для повторения отпустить и нажать снова. Пассивные и каналящиеся спеллы не добавлены в базовые планы.","Start: close menu, select own hero, cursor over visible in-range target, hold the separate binding. Release and hold again to repeat. Passive/channeling spells are not included in basic plans."));
 ImGui::TextWrapped("%s",Status());ImGui::TextWrapped("%s",LastOutcome());if(ImGui::Button(T("Копировать статус прокаста","Copy combo status")))ImGui::SetClipboardText(LastOutcome());
}
}
