from pathlib import Path
import subprocess,shutil,tempfile
root=Path(__file__).resolve().parent.parent
out=root/'build';out.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='esp15-') as name:
 stage=Path(name)
 for f in ['local_visibility.h','common.h','game.h','offsets.h','buff_reader.h','hero_info.h','fog_memory.h','hud_icon_map.inc','ability_max_levels.h']:
  shutil.copy(root/'src'/f,stage/f)
 for f in (root/'tests/effects-mock').iterdir():
  if f.is_file():shutil.copy(f,stage/f.name)
 s=(root/'src/hud.cpp').read_text();a=s.index('bool WorldHeroVisible(');b=s.index('void DrawWorldHero(',a)
 (stage/'production_fog.inc').write_text(s[a:b])
 exe=stage/'render'
 subprocess.run(['g++','-std=c++17','-O1','-I'+str(stage),'-I'+str(root/'vendor/imgui'),str(root/'tests/esp15_render_test.cpp'),*[str(root/'vendor/imgui'/x) for x in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp')],'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=root,check=True)
