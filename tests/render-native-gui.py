from pathlib import Path
import shutil,subprocess
r=Path(__file__).resolve().parent.parent;p=r/'build/gui22-native-stage'
for f in (r/'src').glob('*'):
 if f.suffix in ['.h','.inc'] or f.name in ['menu.cpp','combo_ui.cpp']:shutil.copy(f,p/f.name)
shutil.copytree(r/'src/umbrella',p/'umbrella',dirs_exist_ok=True)
for f in ['Windows.h','mem.h']:shutil.copy(r/'tests/effects-mock'/f,p/f)
with (p/'mem.h').open('a') as f:f.write('\nnamespace rtti {unsigned RuntimeAttempts();unsigned RuntimeParsed();}\n')
(p/'d3d11.h').write_text('#pragma once\nstruct ID3D11Device;struct ID3D11ShaderResourceView;\n');(p/'dxgi.h').write_text('#pragma once\nstruct IDXGISwapChain;\n')
# Full real native UI + full real original renderer. Only game/Windows services mocked.
s=r'''
#include "menu.cpp"
#include "combo_ui.cpp"
#include <iostream>
#include <fstream>
namespace diagnostics {Snapshot snapshot;}
namespace hud {ReadProbe readProbe;}
namespace view {int W=1352,H=765;}
namespace game {Sys g_sys;EntityProbe g_probe;ControllerProbe g_controllerProbe;std::atomic<uintptr_t> g_teamVisibilityData[2];uintptr_t EntityByHandle(uint32_t){return 0;}}
namespace rtti {unsigned RuntimeAttempts(){return 0;}unsigned RuntimeParsed(){return 0;}}
namespace autoaccept {void ClearTemplate(){}std::string Status(){return "Synthetic fixture, no live invite";}}
namespace killstealer {bool BindCurrentHero(){return false;}}
namespace skins {bool SetEnabled(bool){return false;}Status GetStatus(){return {};}void DrawSettings(bool,float){ImGui::TextUnformatted("No connected game in this synthetic fixture");}}
namespace armlet {void Reset(){}const char* Status(){return "Synthetic fixture: no local hero";}const char* StatusRu(){return "Synthetic fixture: no local hero";}const char* LastClosedStatus(){return "No live frame";}const char* TraceDiagnostics(){return "";}const char* SelectionDiagnostics(){return "";}const char* DangerDiagnostics(){return "";}const char* CycleDiagnostics(){return "";}const char* DamageDiagnostics(){return "";}}
namespace combos {void InitializeSettings(){for(int i=0;i<count;++i)for(int j=0;j<profiles[i].count;++j)settings[i].keys[j]=profiles[i].steps[j].defaultKey;}void Reset(){}const char* Status(){return "Synthetic fixture, not a live Dota test";}const char* LastOutcome(){return "No live outcome";}}
'''
# Reuse only draw-mesh exporter, never substitute native settings panels.
old=(r/'build/gui22-stage/preview.cpp').read_text();main=old[old.index('int main('):];main=main.replace('int W=state==1?1920:1352,H=state==1?1080:765;','int W=1352,H=765;').replace('"build/gui22-"','"build/native22-"')
a=main.index(' // Settle');b=main.index(' auto*data=',a)
main=main[:a]+r'''
 clientui::initialized=true;clientui::InitHelpers();cfg::menuOpen=true;
 if(state==0){Menu::Navigate(7,0);s_hostChosen[7]=0;}
 if(state==1){Menu::Navigate(7,0);s_hostChosen[7]=2;combos::settings[1].enabled=true;combos::settings[1].holdKey=VK_SPACE;}
 if(state==2)Menu::Navigate(3,1);
 if(state==3)Menu::Navigate(6,0);
 for(int frame=0;frame<20;++frame){ImGui::NewFrame();DrawMenuWindow();ImGui::Render();}
'''+main[b:]
s=s+main
(p/'native_fixture.cpp').write_text(s)
# Tiny hook allows fixture to select native modules without synthetic replacement panels.
source=(r/'src/menu.cpp').read_text()
(p/'menu.cpp').write_text(source)
exe=p/'native_fixture';cmd=['g++','-std=c++17','-O1','-I'+str(p),'-I'+str(r/'vendor/imgui'),str(p/'native_fixture.cpp'),str(p/'umbrella/umbrella_menu.cpp'),str(p/'umbrella/font_data.cpp'),*[str(r/'vendor/imgui'/f) for f in ['imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp']],'-o',str(exe)]
subprocess.run(cmd,check=True)
for state in range(4):subprocess.run([str(exe),str(state)],cwd=r,check=True)
(r/'tests/raster-native22.py').write_text((r/'tests/raster-gui22.py').read_text().replace("kind=f'gui22-{width}'","kind=f'native22-{width}'"))
print('PASS compiled complete native menu bridge with mocked Windows/game services; not MSVC/live.')
