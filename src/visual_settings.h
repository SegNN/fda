#pragma once
#include "game.h"
#include "visual_controls.h"
namespace visualsettings {
inline bool zoom=false,weather=false,demoConfirmed=false;inline float distance=1400;inline int weatherId=0;
inline char cameraPath[160]={},farzPath[160]={},fogPath[160]={},weatherPath[160]={};
inline visualcontrols::Group camera,climate;inline uintptr_t lastRules=0,lastHero=0;
struct Memory {
 bool Pointer(uintptr_t a,uintptr_t& value){return mem::Read(a,value);}
 bool Read(uintptr_t a,int bytes,uint32_t& n){if(bytes==1){uint8_t x=0;if(!mem::Read(a,x))return false;n=x;return true;}return bytes==4&&mem::Read(a,n);}
 bool Write(uintptr_t a,int bytes,uint32_t n){return bytes==1?mem::Write<uint8_t>(a,uint8_t(n)):bytes==4&&mem::Write<uint32_t>(a,n);}
};
inline void Tick(const Frame& f){
 if((lastRules&&lastRules!=f.rules)||(lastHero&&lastHero!=f.localHandle))demoConfirmed=false;
 if(f.ok){lastRules=f.rules;lastHero=f.localHandle;}
 bool valid=f.ok&&!f.observedOnly&&f.localHandle&&game::g_sys.ready&&demoConfirmed;
 Memory memory;visualcontrols::Path p[3]={visualcontrols::Parse(cameraPath),visualcontrols::Parse(farzPath),visualcontrols::Parse(fogPath)};
 float d=std::clamp(distance,800.f,2400.f);uint32_t values[3]={visualcontrols::FloatBits(d),visualcontrols::FloatBits(d*2.f),0};int widths[3]={4,4,1};
 camera.Tick(memory,zoom,valid,game::g_sys.clientBase,game::g_sys.imageSize,f.rules,p,values,widths,3,false);
 visualcontrols::Path w[1]={visualcontrols::Parse(weatherPath)};uint32_t v[1]={uint32_t(std::clamp(weatherId,0,9))};int size[1]={4};
 climate.Tick(memory,weather,valid,game::g_sys.clientBase,game::g_sys.imageSize,f.rules,w,v,size,1,true);
}
}
