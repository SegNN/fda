from pathlib import Path
import subprocess,shutil
r=Path(__file__).resolve().parent.parent;p=r/'build/gui22-stage';p.mkdir(exist_ok=True)
# Actual entire original menu implementation, including settings/context/gear popups.
code=r'''
#include "umbrella/menu.h"
#include "imgui.h"
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
static bool keys[ImGuiKey_NamedKey_END]{};
static int reloaded=0;
static bool Down(int k){return keys[k];}
static void Reload(){++reloaded;}
static void Native(int rail,int page,float x,float y,float w,float s,int lang){ImGui::SetCursorScreenPos({x,y});ImGui::BeginChild("test native module",{w,470*s});ImGui::TextWrapped("Host callback: existing FDA modules appear here, not an Umbrella game runtime.");ImGui::EndChild();}
int main(int argc,char**argv){int state=argc>1?atoi(argv[1]):0;int W=state==1?1920:1352,H=state==1?1080:765;
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={float(W),float(H)};io.DeltaTime=1.f/60;Menu::Init(125);
 unsigned char*px;int fw,fh;io.Fonts->GetTexDataAsRGBA32(&px,&fw,&fh);io.Fonts->SetTexID((ImTextureID)1);
 std::string stem="build/gui22-"+std::to_string(state);std::ofstream atlas(stem+"-font.rgba",std::ios::binary);atlas.write((char*)px,fw*fh*4);atlas.close();
 // Settle animation by real frames before capturing source draw mesh.
 for(int frame=0;frame<25;++frame){ImGui::NewFrame();Menu::Host(state!=4,Native,Down,Reload);if(state==2&&frame==15)Menu::DebugPopup(1);if(state==3)Menu::Navigate(7,0);if(state==4)keys[ImGuiKey_F7]=frame>=2&&frame<=4;Menu::Frame();ImGui::Render();}
 auto o=Menu::GetOverlay();if(o.items.showOn!=1||o.skills.align!=0||o.items.align!=3||o.bar!=0)return 2;
 if(state==4){if(reloaded!=1 || ImGui::GetDrawData()->TotalVtxCount!=0){std::cerr<<"FAIL closed host backdrop/reload edge "<<reloaded<<" vertices "<<ImGui::GetDrawData()->TotalVtxCount;return 3;}}
 auto*data=ImGui::GetDrawData();std::ofstream out(stem+"-mesh.json");out<<"{\"screen\":["<<W<<","<<H<<"],\"font\":["<<fw<<","<<fh<<"],\"textures\":{},\"lists\":[";
 for(int n=0;n<data->CmdListsCount;++n){if(n)out<<",";auto*dl=data->CmdLists[n];out<<"{\"v\":[";for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}out<<"],\"idx\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"cmd\":[";for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}out<<"]}";}out<<"]}";ImGui::DestroyContext();std::cout<<"PASS full original menu C++ host fixture state "<<state<<"\n";}
'''
(p/'preview.cpp').write_text(code)
exe=p/'preview';subprocess.run(['g++','-std=c++17','-O1','-I'+str(r/'src'),'-I'+str(r/'vendor/imgui'),str(p/'preview.cpp'),str(r/'src/umbrella/umbrella_menu.cpp'),str(r/'src/umbrella/font_data.cpp'),*[str(r/'vendor/imgui'/f) for f in ['imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp']],'-o',str(exe)],check=True)
for state in range(5):subprocess.run([str(exe),str(state)],cwd=r,check=True)
script=(r/'tests/raster-gui21.py').read_text().replace("kind=f'gui21-{width}'","kind=f'gui22-{width}'").replace('actual ESP21','full original ESP22')
(r/'tests/raster-gui22.py').write_text(script)
