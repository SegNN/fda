@echo off
setlocal
cd /d "%~dp0"
for %%F in (src\main.cpp src\game.cpp src\hero_info.h src\ability_max_levels.h src\runtime_rtti.h src\observed_esp.h src\esp_projection.h src\draw.cpp src\hook.cpp src\menu.cpp src\draft_live.cpp src\combos.cpp src\combo_ui.cpp src\combo_core.h src\combo_profiles.h src\combos.h src\npc_visibility.h src\local_visibility.h src\compact_top.h src\draft_live.h src\draft_reader_core.h src\draft_lane_planner.h src\draft_position_weights.inc src\buff_reader.h src\effects_ui.cpp src\effects_ui.h src\armlet.cpp src\armlet.h src\armlet_core.h src\damage_probe.h src\damage_probe_catalog.inc src\helper_hero_catalog.inc src\kill_helper.cpp src\kill_helper.h src\kill_helper_core.h src\kill_helper_catalog.inc src\kill_stealer.cpp src\kill_stealer.h src\kill_stealer_core.h src\auto_accept.cpp src\auto_accept.h src\auto_accept_core.h src\map_events.cpp src\map_events.h src\map_event_tracker.h src\damage_estimate.h src\skin_changer.cpp src\skin_changer_ui.cpp src\skin_changer.h src\cosmetic_core.h src\cosmetic_catalog.inc src\ui_icons.h src\hud.cpp src\top_anchor.cpp src\rtti.cpp src\mem.h src\hook.h src\aegis_tracker.h src\hud_icon_map.inc src\rtti_client.inc vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp vendor\imgui\imgui_impl_win32.cpp vendor\imgui\imgui_impl_dx11.cpp) do (
    if not exist "%%F" (
        echo [-] Missing file: %%F
        echo Extract the FULL project to a new folder. Do not move build.bat alone.
        exit /b 1
    )
)
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS for %%V in (2026 2025 2022 2019 18 17 16) do for %%E in (Community Professional Enterprise BuildTools) do if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo [-] MSVC x64 not found. Install Desktop development with C++ and Windows SDK.
    exit /b 1
)
call "%VCVARS%" >nul 2>&1
if errorlevel 1 exit /b 1
if not exist build mkdir build
if exist build\obsdota2.dll del /q build\obsdota2.dll
if exist build\obsdota2.dll (
    echo [-] Cannot remove old DLL. Close the program using it and retry.
    exit /b 1
)
echo [ESP19] HeroInfo renderer and world data - cosmetics GC disabled
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W3 /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DNOMINMAX /Ivendor\imgui /Fobuild\ src\main.cpp src\game.cpp src\hook.cpp src\draw.cpp src\menu.cpp src\draft_live.cpp src\combos.cpp src\combo_ui.cpp src\effects_ui.cpp src\armlet.cpp src\kill_helper.cpp src\kill_stealer.cpp src\auto_accept.cpp src\map_events.cpp src\skin_changer.cpp src\skin_changer_ui.cpp src\rtti.cpp src\hud.cpp src\top_anchor.cpp vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp vendor\imgui\imgui_impl_win32.cpp vendor\imgui\imgui_impl_dx11.cpp /LD /link /OUT:build\obsdota2.dll d3d11.lib dxgi.lib user32.lib gdi32.lib ole32.lib windowscodecs.lib uuid.lib
if errorlevel 1 (
    if exist build\obsdota2.dll del /q build\obsdota2.dll
    echo [-] Build failed. Do not use an old DLL.
    exit /b 1
)
echo [+] build\obsdota2.dll
exit /b 0
