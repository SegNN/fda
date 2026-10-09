#pragma once
#include <cmath>
#include <algorithm>
namespace worldclip {
struct Rect {float left=0,top=0,right=0,bottom=0;bool valid=false;
 bool Contains(float x,float y,float radius,float below)const{return valid&&std::isfinite(x)&&std::isfinite(y)&&x-radius>=left&&x+radius<=right&&y-radius>=top&&y+below<=bottom;}
};
inline Rect Region(float W,float H,bool cards,float cardY){
 Rect r;if(!std::isfinite(W)||!std::isfinite(H)||W<=0||H<300)return r;
 r.right=W;r.bottom=H*.78f;r.top=cards?std::clamp(std::isfinite(cardY)?cardY:54.f,54.f,H-76.f)+76.f:54.f;
 r.valid=r.top<r.bottom;return r;
}
}
