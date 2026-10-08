#!/usr/bin/env python3
"""Linux mock integration tests. Does not validate real Windows/GDI/Steam/Dota."""
from pathlib import Path
import subprocess,tempfile,shutil
root=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='fda-effects-test-') as name:
    stage=Path(name)
    for f in ('game.cpp','game.h','common.h','offsets.h','hud_icon_map.inc','damage_estimate.h','buff_reader.h','auto_accept.cpp','auto_accept.h','auto_accept_core.h','cosmetic_core.h','kill_stealer.cpp','kill_stealer.h','kill_stealer_core.h','kill_helper.cpp','kill_helper.h','kill_helper_core.h','kill_helper_catalog.inc','armlet.cpp','armlet.h','armlet_core.h'):
        shutil.copy(root/'src'/f,stage/f)
    for f in (root/'tests'/'effects-mock').iterdir():shutil.copy(f,stage/f.name)
    shutil.copy(root/'tests'/'event-mock'/'game_read_test.cpp',stage/'game_read_test.cpp')
    jobs=[(label,root/'tests'/f'{label}_test.cpp') for label in ('buff_reader','kill_stealer_core','auto_accept_core','map_event_tracker','damage_estimate','kill_helper_core','armlet_core','helper_hero_catalog')]
    jobs += [('armlet-integration',stage/'armlet_adapter_test.cpp'),('helper-integration',stage/'kill_helper_adapter_test.cpp'),('automation-integration',stage/'automation_adapter_test.cpp'),('game-memory',stage/'game_read_test.cpp')]
    for label,src in jobs:
        exe=stage/label
        cmd=['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-I'+str(stage),'-I'+str(root/'vendor'/'imgui'),str(src),'-o',str(exe)]
        if label=='helper-integration':cmd += [str(root/'vendor/imgui'/x) for x in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')]
        subprocess.run(cmd,check=True)
        subprocess.run([str(exe)],check=True)
print('PASS mock and portable tests. Live game and Windows build remain untested.')
