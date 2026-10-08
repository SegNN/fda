#!/usr/bin/env python3
"""Linux g++ portable and actual game.cpp MOCK tests. Not Windows/Dota QA."""
from pathlib import Path
import os, shutil, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='fda-events-test-') as name:
    stage=Path(name)
    for f in ('game.cpp','game.h','common.h','offsets.h','hud_icon_map.inc','damage_estimate.h','buff_reader.h'):
        shutil.copy(root/'src'/f,stage/f)
    for f in ('Windows.h','mem.h','game_read_test.cpp'):
        shutil.copy(root/'tests'/'event-mock'/f,stage/f)
    for label,src in [('events',root/'tests'/'map_event_tracker_test.cpp'),('damage',root/'tests'/'damage_estimate_test.cpp'),('game-memory',stage/'game_read_test.cpp')]:
        executable=stage/label
        subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-I'+str(stage),str(src),'-o',str(executable)],check=True)
        subprocess.run([str(executable)],check=True)
print('All event/damage tests passed with ASan/UBSan. Steam, Windows and live Dota were not tested.')
