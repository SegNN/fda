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
