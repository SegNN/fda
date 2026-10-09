#!/usr/bin/env python3
"""Linux mock integration tests. Does not validate real Windows/GDI/Steam/Dota."""
from pathlib import Path
import subprocess,tempfile,shutil,os
root=Path(__file__).resolve().parent.parent
san=[] if os.environ.get('FDA_NO_SANITIZERS')=='1' else ['-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='fda-effects-test-') as name:
    stage=Path(name)
    for f in ('umbrella_style.h','combo_ui.cpp','combos.cpp','combos.h','combo_core.h','combo_profiles.h','compact_top.h','npc_visibility.h','local_visibility.h','game.cpp','runtime_rtti.h','ability_max_levels.h','game.h','hero_info.h','observed_esp.h','esp_projection.h','common.h','offsets.h','hud_icon_map.inc','damage_estimate.h','buff_reader.h','auto_accept.cpp','auto_accept.h','auto_accept_core.h','cosmetic_core.h','kill_stealer.cpp','kill_stealer.h','kill_stealer_core.h','kill_helper.cpp','kill_helper.h','kill_helper_core.h','kill_helper_catalog.inc','armlet.cpp','armlet.h','armlet_core.h','damage_probe.h','damage_probe_catalog.inc','fog_memory.h','draft_advisor.h','draft_roles.inc','draft_live.cpp','draft_live.h','draft_lane_planner.h','draft_position_weights.inc','draft_reader_core.h','helper_hero_catalog.inc'):
        shutil.copy(root/'src'/f,stage/f)
    for f in (root/'tests'/'effects-mock').iterdir():shutil.copy(f,stage/f.name)
    shutil.copy(root/'tests'/'event-mock'/'game_read_test.cpp',stage/'game_read_test.cpp')
    shutil.copy(root/'tests'/'observed_esp_test.cpp',stage/'observed_esp_test.cpp')
    shutil.copy(root/'tests'/'hero_info_test.cpp',stage/'hero_info_test.cpp')
    menu=(root/'src/menu.cpp').read_text()
    (stage/'production_config.inc').write_text(menu[menu.index('static const wchar_t* ConfigPath()'):menu.index('static ImVec4 Accent()')])
    jobs=[(label,root/'tests'/f'{label}_test.cpp') for label in ('buff_reader','kill_stealer_core','auto_accept_core','map_event_tracker','damage_estimate','kill_helper_core','armlet_core','helper_hero_catalog','runtime_rtti','damage_probe','fog_memory','draft_advisor','draft_lane','draft_reader','combo_core','npc_visibility','visual_controls')]
    jobs += [('combo-ui',root/'tests/combo_ui_test.cpp'),('combo-integration',stage/'combo_adapter_test.cpp'),('armlet-integration',stage/'armlet_adapter_test.cpp'),('helper-integration',stage/'kill_helper_adapter_test.cpp'),('automation-integration',stage/'automation_adapter_test.cpp'),('game-memory',stage/'game_read_test.cpp'),('observed-esp',stage/'observed_esp_test.cpp'),('hero-info',stage/'hero_info_test.cpp'),('draft-live',root/'tests/draft_live_test.cpp')]
    requested=os.environ.get('FDA_TEST_FILTER','').split(',')
    if requested!=['']:jobs=[job for job in jobs if job[0] in requested]
    for label,src in jobs:
        exe=stage/label
        cmd=['g++','-std=c++17','-O1','-g',*san,'-I'+str(stage),'-I'+str(root/'vendor'/'imgui'),str(src),'-o',str(exe)]
        if label in ('combo-ui','helper-integration','observed-esp','hero-info'):cmd += [str(root/'vendor/imgui'/x) for x in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')]
        subprocess.run(cmd,check=True)
        subprocess.run([str(exe)],check=True)
# Full original GUI integration has its own ImGui context, native services are IO mocks.
if requested==[''] or 'umbrella-menu' in requested:
    with tempfile.TemporaryDirectory(prefix='fda-original-menu-test-') as name:
        exe=Path(name)/'menu-test'
        subprocess.run(['g++','-std=c++17','-O1','-I'+str(root/'vendor/imgui'),str(root/'tests/umbrella_menu_test.cpp'),str(root/'src/umbrella/font_data.cpp'),*[str(root/'vendor/imgui'/x) for x in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')],'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('PASS mock and portable tests '+('WITH ASan/UBSan' if san else 'WITHOUT ASan/UBSan')+'. Live game and Windows build remain untested.')
