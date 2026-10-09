// Full original production renderer; headless ImGui IO, no Dota/Windows/GPU.
#include "../src/umbrella/umbrella_menu.cpp"
#include <cassert>
#include <iostream>
static bool keyDown[ImGuiKey_NamedKey_END]{};
static int reloadCalls=0;
static bool Down(int k){return keyDown[k];}
static void Reload(){++reloadCalls;}
int main(){ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1352,765};io.DeltaTime=1.f/60;Menu::Init(125);unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);io.Fonts->SetTexID((ImTextureID)1);
 auto frame=[&](bool visible=true){ImGui::NewFrame();Menu::Host(visible,nullptr,Down,Reload);Menu::Frame();ImGui::Render();};
 frame();assert(S>1.02f&&S<1.03f);assert(gMenuPos.x+838*S<=io.DisplaySize.x);assert(gSettingsPos.y+727*S<=io.DisplaySize.y+.01f);
 // Verify the original FA glyph itself; a '?' fallback must never pass preview QA.
 unsigned int cp=0;ImTextCharFromUtf8(&cp,ICON_FA_GEAR,nullptr);assert(FIcon->GetFontBaked(16)->FindGlyphNoFallback((ImWchar)cp));
 // Real mouse events through the original row widget, not assignment to cfg.
 ImVec2 click(gMenuPos.x+U(270),gMenuPos.y+U(115));
 io.AddMousePosEvent(click.x,click.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();assert(!st.itemsEnable);
 // Click original Skills Minified row.
 click={gMenuPos.x+U(620),gMenuPos.y+U(153)};io.AddMousePosEvent(click.x,click.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();assert(st.skillsMinified);
 auto overlay=Menu::GetOverlay();assert(!overlay.items.enabled&&overlay.skills.minified);
 // True host bindings run when menu is closed and toggle once per native edge.
 Bind b;b.path="test.skills";b.label="Skills";b.target=&st.skillsEnable;b.key=ImGuiKey_F6;b.type=0;gBinds.push_back(b);
 frame(false);keyDown[ImGuiKey_F6]=true;frame(false);assert(!st.skillsEnable);frame(false);assert(!st.skillsEnable);keyDown[ImGuiKey_F6]=false;frame(false);keyDown[ImGuiKey_F6]=true;frame(false);assert(st.skillsEnable);
 keyDown[ImGuiKey_F7]=true;frame(false);frame(false);assert(reloadCalls==1);keyDown[ImGuiKey_F7]=false;frame(false);
 // Full settings+gear+bind text round-trip and atomic malformed-input rejection.
 Gear("skills.enable")->size=42;Gear("skills.enable")->opacity=37;st.skillsShowOn=7;st.skillsAlign=2;
 Bind gearBind;gearBind.path="test.gear";gearBind.label="Theme";gearBind.target=&Gear("skills.enable")->themeColors;gearBind.key=ImGuiKey_F5;gBinds.push_back(gearBind);
 std::string saved=Menu::ExportSettings();st.skillsShowOn=1;Gear("skills.enable")->size=20;gBinds.clear();assert(Menu::ImportSettings(saved));
 overlay=Menu::GetOverlay();assert(overlay.skills.showOn==7&&overlay.skills.align==2&&overlay.skills.size==42&&overlay.skills.opacity==37);assert(gBinds.size()==2&&gBinds[0].target==&st.skillsEnable&&gBinds[1].target==&Gear("skills.enable")->themeColors);
 assert(!Menu::ImportSettings("FDA22 1 nan"));assert(!Menu::ImportSettings(saved+" junk"));assert(st.skillsShowOn==7&&gBinds.size()==2);
 gBinds.clear();st.logWindow=false;for(int i=0;i<90;++i)frame(false);assert(ImGui::GetDrawData()->TotalVtxCount==0); // closed menu must not cover game
 ImGui::DestroyContext();std::cout<<"PASS full original Umbrella host: real toggle/minified mouse IO, FA glyph, fitting, closed-menu edge binds/reload, settings+gear+bind persistence, invalid input rejection, zero hidden backdrop. Not Windows/GPU/live.\n";
}
