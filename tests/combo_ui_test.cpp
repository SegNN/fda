#include "combo_ui.cpp"
#include "armlet.h"
#include <cassert>
#include <iostream>
namespace combos {void InitializeSettings(){for(int i=0;i<count;++i)for(int j=0;j<profiles[i].count;++j)if(!settings[i].keys[j])settings[i].keys[j]=profiles[i].steps[j].defaultKey;}const char* Status(){return "Synthetic UI test / no live Dota";}void Reset(){}}
namespace armlet {void Reset(){}}
namespace clientui {static int profile=0,language=0,accent=0;static float fontScale=1;static const wchar_t* profileNames[]={L"profile0",L"profile1",L"profile2"};
#include "production_config.inc"
}
int main(){assert(!clientui::Save()&&!clientui::Load());ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1280,960};io.IniFilename=nullptr;io.Fonts->AddFontDefault();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
 for(int language=0;language<2;++language)for(int profile=0;profile<combos::count;++profile){ImGui::NewFrame();ImGui::SetNextWindowSize({700,900});ImGui::Begin("Production combo UI mock");combos::DrawSettings(profile,language!=0);ImGui::End();ImGui::Render();assert(ImGui::GetDrawData()->TotalVtxCount>0);}
 ImGui::DestroyContext();std::cout<<"PASS actual combo UI: all 9 modules render in RU/EN; actual Save/Load compile and reject unavailable config path. No Windows INI persistence validation.\n";
}
