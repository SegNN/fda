from pathlib import Path
import sys,json,hashlib
from parse_catalog import parse
root=Path(__file__).resolve().parent.parent;inp=root/'tests/helper-kv'
# Explicit review list: one impact only, never sum DoT/channel/multicast.
rows=[
('lina','dragon_slave','Dragon Slave','dragon_slave_damage',None),
('lina','light_strike_array','Light Strike Array','light_strike_array_damage',None),
('lina','laguna_blade','Laguna Blade','damage',None),
('lion','impale','Earth Spike','damage',None),
('lion','finger_of_death','Finger of Death','damage',None),
('zuus','arc_lightning','Arc Lightning','arc_damage',None),
('zuus','lightning_bolt','Lightning Bolt','damage',None),
('luna','lucent_beam','Lucent Beam','beam_damage',None),
('sven','storm_bolt','Storm Hammer','@AbilityDamage',None),
('ogre_magi','fireblast','Fireblast','fireblast_damage',None),
('pudge','meat_hook','Meat Hook','damage',None),
('queenofpain','shadow_strike','Shadow Strike (impact)','strike_damage',None),
('queenofpain','scream_of_pain','Scream of Pain','damage','area_of_effect'),
('queenofpain','sonic_wave','Sonic Wave','damage',None),
('vengefulspirit','magic_missile','Magic Missile','magic_missile_damage',None),
('vengefulspirit','wave_of_terror','Wave of Terror','damage',None),
('vengefulspirit','nether_swap','Nether Swap','damage',None)]
def vals(x):
 if isinstance(x,dict):x=x['value']
 return [float(n) for n in x.split()]
def arr(x,n):
 assert len(x) in [1,n] and 0<n<=4
 return x*n if len(x)==1 else x
out=[];meta=[]
for hero,short,label,key,rkey in rows:
 p=inp/f'npc_dota_hero_{hero}.txt';skill=f'{hero}_{short}';d=parse(p.read_text())['DOTAHeroes'][f'npc_dota_hero_{hero}']['AbilityDefinitions'][skill];v=d['AbilityValues']
 damage=vals(d[key[1:]] if key.startswith('@') else v[key]);n=len(damage)
 if n==1:n=4 # not currently used
 damage=arr(damage,n)
 range_path=f'AbilityValues.{rkey}' if rkey else ('AbilityValues.AbilityCastRange' if 'AbilityCastRange' in v else 'AbilityCastRange')
 reach=arr(vals(v[rkey] if rkey else v.get('AbilityCastRange',d.get('AbilityCastRange'))),n)
 typ={'DAMAGE_TYPE_MAGICAL':0,'DAMAGE_TYPE_PHYSICAL':1,'DAMAGE_TYPE_PURE':2}[d['AbilityUnitDamageType']]
 def emit(xs):return '{'+','.join(f'{x:.8g}f' if '.' in f'{x:.8g}' else f'{x:.8g}.f' for x in xs+[0]*(4-len(xs)))+'}'
 out.append('{"npc_dota_hero_'+hero+'","'+skill+'","'+label+'",'+str(typ)+','+str(n)+','+emit(damage)+','+emit(reach)+'},')
 meta.append(dict(hero=hero,ability=skill,label=label,damage_path=key[1:] if key.startswith('@') else 'AbilityValues.'+key,range_path=range_path,damage=damage,range=reach,type=typ,sha256=hashlib.sha256(p.read_bytes()).hexdigest(),source='https://github.com/SteamDatabase/GameTracking-Dota2/blob/5c3ea270b75b892ea607d0c236222a3f0ee34ad1/game/dota/pak01_dir/scripts/npc/heroes/'+p.name))
(root/'src/kill_helper_catalog.inc').write_text('// Generated reviewed BASE impact values; not live final spell damage.\n'+'\n'.join(out)+'\n')
(root/'tests/kill-helper-catalog-sources.json').write_text(json.dumps({'commit':'5c3ea270b75b892ea607d0c236222a3f0ee34ad1','models':meta,'exclusions':['facets','talents','scepter','shard','spell amplification','damage barriers','damage over time','multicast','kill stacks','flight/aim','regen']},indent=2)+'\n')
print('Generated',len(out),'explicit baseline impact models')
