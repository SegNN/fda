from pathlib import Path
import re,json,hashlib
root=Path(__file__).resolve().parent.parent
roles=['Carry','Support','Nuker','Disabler','Jungler','Durable','Escape','Pusher','Initiator']
lines=['// Published KV role strengths; not lane assignments or win rates.\n']; sources=[]
for label,engine,id in re.findall(r'\{"([^"]+)","([^"]+)",(\d+)\}',(root/'src/helper_hero_catalog.inc').read_text()):
 file=root/'tests/helper-hero-kv'/f'{engine}.txt';text=file.read_text();r=re.search(r'"Role"\s+"([^"]+)"',text);l=re.search(r'"Rolelevels"\s+"([^"]+)"',text)
 if not r or not l:raise ValueError('Missing roles: '+engine)
 names=r.group(1).split(',');strengths=list(map(int,l.group(1).split(',')))
 if len(names)!=len(strengths) or any(x<0 or x>3 for x in strengths):raise ValueError(engine)
 values=dict(zip(names,strengths));lines.append('{"%s",%s,{%s}},\n'%(label,id,','.join(str(values.get(x,0)) for x in roles)))
 sources.append({'file':file.name,'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
(root/'src/draft_roles.inc').write_text(''.join(lines));provenance=json.loads((root/'tests/helper-hero-sources.json').read_text())
(root/'DRAFT-ADVISOR-SOURCES.json').write_text(json.dumps({'commit':provenance['commit'],'base_url':provenance['base_url'],'fields':['Role','Rolelevels'],'count':len(sources),'roles':roles,'algorithm':'Heuristic sum of min(candidate role strength, max(0, coverage target - ally role sum)); targets [4,4,3,4,0,3,0,2,3]. Stable tie order follows alphabetic roster. Not lane assignments, winrate or meta. Manual picks only.','sources':sources},indent=2)+'\n')
print('Validated role metadata and provenance for',len(sources),'heroes.')
