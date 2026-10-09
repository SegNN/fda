Umbrella Menu — Dear ImGui (Win32 + DirectX 11)

Run:   UmbrellaMenu.exe   (Windows 10/11 x64, no install needed)
INS          show / hide the menu (rebind in Settings -> Menu Bind)
Gear (bottom left of the rail)  open / close the Settings window
Gear next to a toggle           settings popup (Theme Colors / Size / Opacity / Hover Opacity)
Right click on any row          context menu: New Bind / Bind List / Copy Lua Path / Reset
Settings -> Language            en / ru,  Menu Scale 100-200,  Theme swatches, "+" = custom color

Build from source (Visual Studio 2019/2022 + CMake):
  cmake -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
Code: src/menu.cpp (the whole menu), src/main_win32.cpp (DX11 window).
Fonts (Lato + Font Awesome 6 Free) are embedded in src/font_data.cpp.
