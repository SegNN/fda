#pragma once
#include "game.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace observedesp {
struct Stats {int considered=0, projected=0, drawn=0;};
template<class Project>
bool HeroBox(const FrameUnit& u, ImDrawList* dl, Project project, int height, ImU32 color) {
    if(!dl||!u.alive)return false;
    ImVec2 foot,top;Vec3 upper{u.pos.x,u.pos.y,u.pos.z+u.hbOffset};
    if(!project(u.pos,foot)||!project(upper,top))return false;
    float h=foot.y-top.y;if(!std::isfinite(h)||h<8.f||h>height*1.2f)return false;
    float width=std::clamp(h*.42f,18.f,180.f);
    ImVec2 a(top.x-width*.5f,top.y),b(top.x+width*.5f,foot.y);
    float len=std::max(5.f,std::min(width,h)*.28f);
    dl->AddLine(a,ImVec2(a.x+len,a.y),color,1.6f);dl->AddLine(a,ImVec2(a.x,a.y+len),color,1.6f);
    dl->AddLine(ImVec2(b.x,a.y),ImVec2(b.x-len,a.y),color,1.6f);dl->AddLine(ImVec2(b.x,a.y),ImVec2(b.x,a.y+len),color,1.6f);
    dl->AddLine(ImVec2(a.x,b.y),ImVec2(a.x+len,b.y),color,1.6f);dl->AddLine(ImVec2(a.x,b.y),ImVec2(a.x,b.y-len),color,1.6f);
    dl->AddLine(b,ImVec2(b.x-len,b.y),color,1.6f);dl->AddLine(b,ImVec2(b.x,b.y-len),color,1.6f);
    return true;
}

// Routes verified geometry to the EXISTING project renderers. No diagnostic skin/text/bars.
template<class Project,class Hero,class Ward,class Roshan>
Stats Draw(const Frame& frame, ImDrawList* dl, Project project, int width, int height,
           bool heroes,bool wards,bool roshan,Hero heroRenderer,Ward wardRenderer,Roshan roshanRenderer) {
    Stats stats;
    if(!frame.observedOnly||!dl||width<=0||height<=0)return stats;
    for(const auto& u:frame.units) {
        bool selected=(u.kind==UnitKind::Hero&&heroes)||(u.kind==UnitKind::Ward&&wards)||
                      (u.kind==UnitKind::Roshan&&roshan);
        if(!selected||!u.alive||u.maxHp<=0||u.hp<=0||u.hp>u.maxHp)continue;
        ++stats.considered;
        ImVec2 foot,head;Vec3 upper{u.pos.x,u.pos.y,u.pos.z+u.hbOffset};
        if(!project(u.pos,foot)||!project(upper,head))continue;
        ++stats.projected;float h=foot.y-head.y;
        if(!std::isfinite(h)||h<8.f||h>height*1.2f||foot.x<0||foot.x>width||foot.y<0||head.y>height)continue;
        if(u.kind==UnitKind::Hero)heroRenderer(u);
        else if(u.kind==UnitKind::Ward)wardRenderer(u);
        else roshanRenderer(u);
        ++stats.drawn;
    }
    return stats;
}
}
