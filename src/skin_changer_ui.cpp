#include "skin_changer.h"
#include "imgui.h"
namespace skins {
void DrawSettings(bool english, float scale) {
    (void)scale;
    ImGui::TextWrapped("%s", english
        ? "Cosmetics are disabled in this temporary build. No virtual items, Steam GC interception or game-file access. This is NOT a replacement skin changer."
        : "Косметика отключена в этой временной сборке. Нет виртуальных предметов, перехвата Steam GC и доступа к файлам игры. Это НЕ новый скинченджер.");
    ImGui::TextWrapped("%s", english
        ? "Auto-accept also loses its GC ready-up signal in this build. Accept matches manually. Restart Dota before using the rebuilt DLL."
        : "Auto-accept также не получает сигнал готовности от GC в этой сборке. Принимай матчи вручную. Перед использованием новой DLL полностью перезапусти Dota.");
}
}
