// Original user-provided Lotus SVG paths, flattened to native ImGui polylines.
#pragma once
#include "imgui.h"
namespace uiicons {
enum Icon {Check,Keyboard,Sliders,Render,Combat,Map,Grid,Configs,Settings,Diagnostics,Assist};
inline void Draw(ImDrawList* dl,ImVec2 center,float size,ImU32 color,Icon icon){
 const float scale=size/24.f;const ImVec2 origin(center.x-size*.5f,center.y-size*.5f);
 auto point=[&](float x,float y){return ImVec2(origin.x+x*scale,origin.y+y*scale);};
 switch(icon){
 case Assist:
 dl->AddCircle(point(8,7),3*scale,color,24,1.75f*scale);
 dl->AddLine(point(2,20),point(2,17),color,1.75f*scale);dl->AddLine(point(2,17),point(8,13),color,1.75f*scale);dl->AddLine(point(8,13),point(12,16),color,1.75f*scale);
 {const ImVec2 v[]={point(19,3),point(14,12),point(19,12),point(15,21),point(23,10),point(18,10)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,ImDrawFlags_Closed,1.75f*scale);}break;
 case (Icon)0:
 {const ImVec2 v[]={point(5.000f,12.000f),point(9.500f,16.500f),point(19.000f,7.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)1:
 dl->AddRect(point(3.000f,5.000f),point(21.000f,19.000f),color,2.000f*scale,0,1.75f*scale);
 dl->AddCircleFilled(point(7.000f,9.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(11.000f,9.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(15.000f,9.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(18.000f,9.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(7.000f,12.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(11.000f,12.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(15.000f,12.000f),.875f*scale,color,12);
 dl->AddCircleFilled(point(18.000f,12.000f),.875f*scale,color,12);
 {const ImVec2 v[]={point(7.000f,15.000f),point(17.000f,15.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)2:
 {const ImVec2 v[]={point(3.000f,6.000f),point(7.000f,6.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(11.000f,6.000f),point(21.000f,6.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(3.000f,12.000f),point(13.000f,12.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(17.000f,12.000f),point(21.000f,12.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(3.000f,18.000f),point(7.000f,18.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(11.000f,18.000f),point(21.000f,18.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 dl->AddCircle(point(9.0f,6.0f),2.0f*scale,color,24,1.75f*scale);
 dl->AddCircle(point(15.0f,12.0f),2.0f*scale,color,24,1.75f*scale);
 dl->AddCircle(point(9.0f,18.0f),2.0f*scale,color,24,1.75f*scale);
 break;
 case (Icon)3:
 dl->AddRect(point(3.000f,3.000f),point(21.000f,21.000f),color,2.000f*scale,0,1.75f*scale);
 dl->AddCircle(point(8.0f,8.0f),2.0f*scale,color,24,1.75f*scale);
 {const ImVec2 v[]={point(3.000f,17.000f),point(8.000f,12.000f),point(12.000f,16.000f),point(16.000f,10.000f),point(21.000f,18.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)4:
 {const ImVec2 v[]={point(4.000f,3.000f),point(9.000f,5.000f),point(20.000f,17.000f),point(17.000f,20.000f),point(5.000f,8.000f),point(4.000f,3.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,ImDrawFlags_Closed,1.75f*scale);}
 {const ImVec2 v[]={point(3.000f,17.000f),point(7.000f,21.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(4.000f,20.000f),point(8.000f,16.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(20.000f,3.000f),point(15.000f,5.000f),point(12.000f,8.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(16.000f,12.000f),point(19.000f,8.000f),point(20.000f,3.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(15.000f,16.000f),point(20.000f,20.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(17.000f,21.000f),point(21.000f,17.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)5:
 dl->AddCircle(point(12.0f,12.0f),9.0f*scale,color,24,1.75f*scale);
 {const ImVec2 v[]={point(3.000f,12.000f),point(21.000f,12.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(12.000f,3.000f),point(10.854f,4.309f),point(9.917f,5.722f),point(9.188f,7.219f),point(8.667f,8.778f),point(8.354f,10.378f),point(8.250f,12.000f),point(8.354f,13.622f),point(8.667f,15.222f),point(9.188f,16.781f),point(9.917f,18.278f),point(10.854f,19.691f),point(12.000f,21.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(12.000f,3.000f),point(13.146f,4.309f),point(14.083f,5.722f),point(14.812f,7.219f),point(15.333f,8.778f),point(15.646f,10.378f),point(15.750f,12.000f),point(15.646f,13.622f),point(15.333f,15.222f),point(14.812f,16.781f),point(14.083f,18.278f),point(13.146f,19.691f),point(12.000f,21.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)6:
 dl->AddRect(point(3.000f,3.000f),point(10.000f,10.000f),color,1.500f*scale,0,1.75f*scale);
 dl->AddRect(point(14.000f,3.000f),point(21.000f,10.000f),color,1.500f*scale,0,1.75f*scale);
 dl->AddRect(point(3.000f,14.000f),point(10.000f,21.000f),color,1.500f*scale,0,1.75f*scale);
 dl->AddRect(point(14.000f,14.000f),point(21.000f,21.000f),color,1.500f*scale,0,1.75f*scale);
 break;
 case (Icon)7:
 {const ImVec2 v[]={point(14.000f,3.000f),point(6.000f,3.000f),point(5.729f,3.018f),point(5.468f,3.071f),point(5.222f,3.157f),point(4.991f,3.273f),point(4.778f,3.417f),point(4.586f,3.586f),point(4.417f,3.778f),point(4.273f,3.991f),point(4.157f,4.222f),point(4.071f,4.468f),point(4.018f,4.729f),point(4.000f,5.000f),point(4.000f,19.000f),point(4.018f,19.271f),point(4.071f,19.532f),point(4.157f,19.778f),point(4.273f,20.009f),point(4.417f,20.222f),point(4.586f,20.414f),point(4.778f,20.583f),point(4.991f,20.727f),point(5.222f,20.843f),point(5.468f,20.929f),point(5.729f,20.982f),point(6.000f,21.000f),point(18.000f,21.000f),point(18.271f,20.982f),point(18.532f,20.929f),point(18.778f,20.843f),point(19.009f,20.727f),point(19.222f,20.583f),point(19.414f,20.414f),point(19.583f,20.222f),point(19.727f,20.009f),point(19.843f,19.778f),point(19.929f,19.532f),point(19.982f,19.271f),point(20.000f,19.000f),point(20.000f,9.000f),point(14.000f,3.000f),point(14.000f,9.000f),point(20.000f,9.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(8.000f,13.000f),point(16.000f,13.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(8.000f,17.000f),point(13.000f,17.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 case (Icon)8:
 {const ImVec2 v[]={point(10.000f,3.000f),point(9.000f,6.000f),point(7.000f,7.000f),point(4.000f,6.500f),point(3.000f,9.500f),point(5.500f,11.500f),point(5.500f,13.500f),point(3.000f,15.000f),point(4.000f,18.000f),point(7.000f,17.500f),point(9.000f,18.500f),point(10.000f,20.500f),point(14.000f,20.500f),point(15.000f,18.500f),point(17.000f,17.500f),point(20.000f,18.000f),point(21.000f,15.000f),point(18.500f,13.500f),point(18.500f,11.500f),point(21.000f,9.000f),point(20.000f,6.000f),point(17.000f,6.500f),point(15.000f,5.500f),point(14.000f,3.500f),point(10.000f,3.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,ImDrawFlags_Closed,1.75f*scale);}
 dl->AddCircle(point(12.0f,12.0f),3.0f*scale,color,24,1.75f*scale);
 break;
 case (Icon)9:
 {const ImVec2 v[]={point(8.000f,6.000f),point(2.000f,12.000f),point(8.000f,18.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(16.000f,6.000f),point(22.000f,12.000f),point(16.000f,18.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 {const ImVec2 v[]={point(14.000f,3.000f),point(10.000f,21.000f)};dl->AddPolyline(v,IM_ARRAYSIZE(v),color,0,1.75f*scale);}
 break;
 }
}
}