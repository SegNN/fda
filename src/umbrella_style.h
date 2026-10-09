#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>
// Adapted geometry/palette/toggle treatment from the user-provided UmbrellaMenu.
// Existing FDA ImGui/backends retained; standalone EXE and demo feature flags not used.
namespace umbrellastyle {
inline ImU32 Alpha(ImU32 c,int a){return (c&0x00ffffffu)|(ImU32(std::clamp(a,0,255))<<24);}
inline void Halo(ImDrawList* dl,ImVec2 a,ImVec2 b,ImU32 color,float rounding=6,float strength=1){
 for(int i=5;i>=1;--i){float e=float(i);dl->AddRect({a.x-e,a.y-e},{b.x+e,b.y+e},Alpha(color,int((12-i)*strength)),rounding+e,0,1);}
}
inline void Shadow(ImDrawList* dl,ImVec2 a,ImVec2 b,float rounding=6){
 for(int i=10;i>=1;--i){float e=i*1.5f;dl->AddRectFilled({a.x-e,a.y-e+4},{b.x+e,b.y+e+4},IM_COL32(0,0,0,9),rounding+e);}
}
inline void Toggle(ImDrawList* dl,ImGuiID id,float right,float cy,bool on,ImU32 accent){
 auto* storage=ImGui::GetStateStorage();float t=storage->GetFloat(id,on?1.f:0.f);t+=(float(on)-t)*std::min(1.f,ImGui::GetIO().DeltaTime*16.f);storage->SetFloat(id,t);
 ImVec2 a{right-30,cy-8},b{right,cy+8};dl->AddRectFilled(a,b,IM_COL32(25,27,33,255),8);dl->AddRectFilled(a,b,Alpha(accent,int(45*t)),8);
 float x=a.x+8+14*t;if(on)Halo(dl,{x-6,cy-6},{x+6,cy+6},accent,8,.6f);
 dl->AddCircleFilled({x,cy},7,on?accent:IM_COL32(102,108,121,255),32);
}
}
