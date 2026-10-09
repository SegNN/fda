from pathlib import Path
import shutil,tempfile,subprocess
r=Path(__file__).resolve().parent.parent;s=(r/'src/menu.cpp').read_text();p=r/'build/gui21-stage';p.mkdir(parents=True,exist_ok=True)
for f in ['game.h','common.h','offsets.h','buff_reader.h','ui_icons.h','umbrella_style.h','combos.h','combo_core.h','combo_profiles.h','combo_ui.cpp','visual_controls.h','visual_settings.h']:shutil.copy(r/'src'/f,p/f)
for f in (r/'tests/effects-mock').iterdir():
 if f.is_file():shutil.copy(f,p/f.name)
style=s[s.index('static ImVec4 UI_RGB'):s.index('static bool s_capture')]
helpers=s[s.index('static ImVec4 Accent()'):s.index('static void Unsupported(')]
tables=s[s.index('namespace clientui {\nstruct Function'):s.index('void DrawMenuWindow()')]
shell=s[s.index('void DrawMenuWindow()'):s.index('    if((fn.details<15')]
visual=s[s.index('    case 24:case 25:'):s.index('    case 23:')]
body=r'''    Card("preview-options","Настройки","Settings",0);
    if(category==7&&selected>0)combos::DrawSettings(selected-1,language!=0);
    else if(category==7){Row("Включить Armlet для своего героя","Enable own-hero Armlet",&cfg::armletAuto);Row("Клавиша проверена в демо","Native key tested in demo",&cfg::armletConfirmed);LotusSlider(T("Переключать при HP <=","Toggle when HP <="),&cfg::armletThreshold,50,550,"%.0f HP");Help("Только свой герой с Armlet в выбранном слоте. Высокий HP не блокирует прокаст. Нужны закрытое меню и фокус Dota.","Own hero with Armlet in configured slot. High HP does not block combos. Close menu and focus Dota.");}
    else if(category==3){switch(fn.details){VISUAL_CASES}}
    else{Row("Способности и уровни","Abilities & levels",&cfg::hudAbilities);Row("Предметы и заряды","Items & charges",&cfg::hudItems);Row("Число HP","HP number",&cfg::hudHpNumber);Row("Статусы / INVIS","Statuses / INVIS",&cfg::hudStatusBadges);LotusSlider(T("Размер иконок","Icon size"),&cfg::hudSkillPixels,28,60,"%.0f px");Help("В тумане — последняя видимая позиция. Камера не меняет WORLD координаты метки. Верхние карточки: HP, мана и скиллы с КД.","Fog uses last-seen position. Camera does not change the marker WORLD point. Top cards: HP, mana, skill cooldowns only.");}
    EndCard();ImGui::EndChild();ImGui::End();
}
'''
body=body.replace('VISUAL_CASES',visual).replace('—','-')
preamble=r'''#include "game.h"
#include "imgui_internal.h"
#include "ui_icons.h"
#include "umbrella_style.h"
#include "visual_settings.h"
#include "combo_ui.cpp"
#include <string>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <iostream>
namespace theme {void ApplyStyle();}
namespace combos {void InitializeSettings(){for(int i=0;i<count;++i)for(int j=0;j<profiles[i].count;++j)settings[i].keys[j]=profiles[i].steps[j].defaultKey;}const char* Status(){return "Demo fixture: ready for configured hold binding";}const char* LastOutcome(){return "Synthetic preview, not a live Dota capture";}}
namespace clientui {static int language=0,accent=1,profile=0;static float fontScale=1;static const char* profileLabels[]={"default","practice","custom"};static char message[160]={};static bool initialized=true;static int s_openReq=-1;static const char* T(const char* ru,const char* en){return language?en:ru;}static const wchar_t* ConfigPath(){return L"";}static bool Load(){return false;}static bool Save(){return false;}}
'''
(p/'preview.cpp').write_text(preamble+style+'\nnamespace clientui {\n'+helpers+'\n}\n'+tables+shell+body+r'''
int main(int argc,char**argv){int state=argc>1?atoi(argv[1]):0;int W=state==0?1280:1920,H=state==0?800:1080;ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={float(W),float(H)};io.IniFilename=nullptr;io.DeltaTime=1.f/60;auto*font=io.Fonts->AddFontFromFileTTF("assets/fonts/ui-semibold.otf",15,nullptr,io.Fonts->GetGlyphRangesCyrillic());if(!font)io.Fonts->AddFontDefault();unsigned char*px;int fw,fh;io.Fonts->GetTexDataAsRGBA32(&px,&fw,&fh);io.Fonts->SetTexID((ImTextureID)1);std::string stem="build/gui21-"+std::to_string(state);std::ofstream atlas(stem+"-font.rgba",std::ios::binary);atlas.write((char*)px,fw*fh*4);atlas.close();cfg::menuOpen=true;cfg::armletAuto=cfg::armletConfirmed=true;clientui::InitHelpers();if(state){clientui::category=state>=3?3:7;clientui::selected=state>=3?state:state==1?0:2;}if(state==2){combos::settings[1].enabled=true;combos::settings[1].holdKey=VK_SPACE;}
ImGui::NewFrame();auto*bg=ImGui::GetBackgroundDrawList();bg->AddRectFilled({0,0},{float(W),float(H)},IM_COL32(8,12,18,255));DrawMenuWindow();ImGui::Render();auto*data=ImGui::GetDrawData();std::ofstream out(stem+"-mesh.json");out<<"{\"screen\":["<<W<<","<<H<<"],\"font\":["<<fw<<","<<fh<<"],\"textures\":{},\"lists\":[";
for(int n=0;n<data->CmdListsCount;++n){if(n)out<<",";auto*dl=data->CmdLists[n];out<<"{\"v\":[";for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}out<<"],\"idx\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"cmd\":[";for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}out<<"]}";}out<<"]}";ImGui::DestroyContext();std::cout<<"PASS source-extracted ESP21 shell/widgets preview state "<<state<<"; fixture details, not Windows/live runtime\n";}
''')
exe=p/'preview';subprocess.run(['g++','-std=c++17','-O1','-I'+str(p),'-I'+str(r/'vendor/imgui'),str(p/'preview.cpp'),*[str(r/'vendor/imgui'/f) for f in ['imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp']],'-o',str(exe)],check=True)
for state in range(5):subprocess.run([str(exe),str(state)],cwd=r,check=True)
script=(r/'tests/raster-esp21.py').read_text().replace("kind=f'esp21-{width}'+('-hidden' if len(sys.argv)>2 else '')","kind=f'gui21-{width}'")
(r/'tests/raster-gui21.py').write_text(script)
