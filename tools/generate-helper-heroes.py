"""Reproduce hero list from included read-only KV snapshot, not from installed Dota."""
from pathlib import Path
import json,hashlib
from parse_catalog import parse
root=Path(__file__).resolve().parent.parent
rows=[];sources=[]
for p in (root/'tests/helper-hero-kv').glob('npc_dota_hero_*.txt'):
    name=p.stem;data=parse(p.read_text())['DOTAHeroes'][name]
    if str(data.get('Enabled','1'))=='0':continue
    label=data.get('workshop_guide_name',name.removeprefix('npc_dota_hero_').replace('_',' ').title())
    rows.append((label,name,int(data['HeroID'])))
    sources.append({'file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
rows.sort(key=lambda x:x[0].casefold())
assert len({x[1] for x in rows})==len(rows)==len({x[2] for x in rows})
(root/'src/helper_hero_catalog.inc').write_text('// Enabled heroes from pinned KV snapshot; only Huskar has an implementation.\n'+'\n'.join('{'+json.dumps(label)+','+json.dumps(name)+','+str(i)+'},' for label,name,i in rows)+'\n')
(root/'tests/helper-hero-sources.json').write_text(json.dumps({'commit':'5c3ea270b75b892ea607d0c236222a3f0ee34ad1','base_url':'https://github.com/SteamDatabase/GameTracking-Dota2/tree/5c3ea270b75b892ea607d0c236222a3f0ee34ad1/game/dota/pak01_dir/scripts/npc/heroes','fields':['Enabled','workshop_guide_name','HeroID'],'count':len(rows),'sources':sorted(sources,key=lambda x:x['file'])},indent=2)+'\n')
print('Generated',len(rows),'source hero entries')
