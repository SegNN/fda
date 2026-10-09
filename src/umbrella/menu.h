#pragma once
// Umbrella-style ImGui menu (Heroes Overlay replica)
namespace Menu
{
    // default_scale: one of 100,125,150,175,200 ("Menu Scale" option)
    void  Init(int default_scale);
    // Call between ImGui::NewFrame() and ImGui::Render()
    void  Frame();
    // Background clear color (r,g,b)
    void  ClearColor(float* rgb);
    // Size (in pixels) needed for the menu + settings window at a given scale
    void  RequiredSize(int scale, float* w, float* h);
    // For automated previews / tests
    void  DebugOpen(int what);
}
namespace Menu
{
    void  SetWindowPositions(float menu_x, float menu_y, float settings_x, float settings_y);
    void  SetSettingsOpen(bool open);
}

namespace Menu {
struct Layer { bool enabled=true,minified=false,themeColors=true; int showOn=7,align=0; float size=29,opacity=100,hoverOpacity=60; };
struct Overlay { Layer items,skills,modifiers; bool bars=true; int bar=0; float hoverRadius=110; unsigned accent=0; };
using PageCallback=void(*)(int,int,float,float,float,float,int);
using KeyCallback=bool(*)(int);
void Host(bool visible,PageCallback page,KeyCallback down,void(*reload)());
Overlay GetOverlay(); void SetEnabled(bool items,bool skills,bool mods,bool bars);
int MenuKey(); int Language(); bool Visible();
void LogHost(const char* message);
void Navigate(int rail,int page);
void DebugPopup(int which);
}

#include <string>
namespace Menu {std::string ExportSettings();bool ImportSettings(const std::string& text);}
