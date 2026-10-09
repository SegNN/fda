#pragma once
#include "game.h"
#include <cmath>
namespace espprojection {
struct Choice { int mode=0, uprightVotes=0, flippedVotes=0; };
inline bool Project(const float* m,int mode,const Vec3& w,int width,int height,float& sx,float& sy,float& pw) {
    if(!m||width<=0||height<=0)return false;
    float px,py;
    if(mode&2){px=m[0]*w.x+m[4]*w.y+m[8]*w.z+m[12];py=m[1]*w.x+m[5]*w.y+m[9]*w.z+m[13];pw=m[3]*w.x+m[7]*w.y+m[11]*w.z+m[15];}
    else{px=m[0]*w.x+m[1]*w.y+m[2]*w.z+m[3];py=m[4]*w.x+m[5]*w.y+m[6]*w.z+m[7];pw=m[12]*w.x+m[13]*w.y+m[14]*w.z+m[15];}
    if(!std::isfinite(px)||!std::isfinite(py)||!std::isfinite(pw)||pw<=1e-6f)return false;
    float nx=px/pw,ny=py/pw;if(mode&1)ny=-ny;
    sx=(nx*.5f+.5f)*width;sy=(.5f-ny*.5f)*height;
    return std::isfinite(sx)&&std::isfinite(sy);
}
inline Choice Resolve(const float* m,int width,int height,const Frame& frame,int previousLayout=0) {
    Choice c;
    float a=std::fabs(m[12])+std::fabs(m[13])+std::fabs(m[14]);
    float b=std::fabs(m[3])+std::fabs(m[7])+std::fabs(m[11]);
    c.mode=a<b*.5f?0:b<a*.5f?2:(previousLayout&2);
    auto vote=[&](const Vec3& pos) {
        float x,y,w,tx,ty,tw;Vec3 top{pos.x,pos.y,pos.z+150.f};
        if(!Project(m,c.mode,pos,width,height,x,y,w)||!Project(m,c.mode,top,width,height,tx,ty,tw))return;
        // An off-camera point can reverse d(screenY)/d(worldZ) through perspective.
        // It must NEVER decide the camera's global Y direction (ESP2 zero-origin bug).
        if(x<0||x>width||y<0||y>height||tx<0||tx>width||ty<0||ty>height)return;
        float delta=ty-y;if(std::fabs(delta)<2.f||std::fabs(delta)>height*.6f)return;
        if(delta<0)++c.uprightVotes;else ++c.flippedVotes;
    };
    if(frame.ok)vote(frame.localPos);
    for(const auto& u:frame.units)if(u.alive&&u.kind==UnitKind::Hero)vote(u.pos);
    if(c.flippedVotes>c.uprightVotes)c.mode|=1;
    // No votes/tie: conventional unflipped Y, not stale orientation or an arbitrary hidden hero.
    return c;
}
}
