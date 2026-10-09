#pragma once
namespace combos {
struct Step {const char* ability;int defaultKey;float range;};
struct Profile {const char* label;const char* hero;Step steps[3];int count;};
inline const Profile profiles[]={
 {"Lion / basic burst","npc_dota_hero_lion",{{"lion_voodoo",'W',500},{"lion_impale",'Q',500},{"lion_finger_of_death",'R',500}},3},
 {"Lina / basic burst","npc_dota_hero_lina",{{"lina_light_strike_array",'W',500},{"lina_dragon_slave",'Q',500},{"lina_laguna_blade",'R',500}},3},
 {"Sven / basic burst","npc_dota_hero_sven",{{"sven_gods_strength",'R',500},{"sven_storm_bolt",'Q',500},{"sven_warcry",'E',500}},3},
 {"Vengeful Spirit / basic burst","npc_dota_hero_vengefulspirit",{{"vengefulspirit_magic_missile",'Q',400},{"vengefulspirit_wave_of_terror",'W',400},{}},2},
 {"Ogre Magi / basic burst","npc_dota_hero_ogre_magi",{{"ogre_magi_fireblast",'Q',400},{"ogre_magi_ignite",'W',400},{}},2},
 {"Zeus / basic burst","npc_dota_hero_zuus",{{"zuus_lightning_bolt",'W',400},{"zuus_arc_lightning",'Q',400},{"zuus_thundergods_wrath",'R',400}},3},
 {"Queen of Pain / basic burst","npc_dota_hero_queenofpain",{{"queenofpain_shadow_strike",'Q',350},{"queenofpain_sonic_wave",'R',350},{"queenofpain_scream_of_pain",'E',350}},3},
 {"Luna / basic burst","npc_dota_hero_luna",{{"luna_lucent_beam",'Q',350},{"luna_eclipse",'R',350},{}},2},
 {"Dragon Knight / basic burst","npc_dota_hero_dragon_knight",{{"dragon_knight_dragon_tail",'W',125},{"dragon_knight_breathe_fire",'Q',125},{}},2},
};
inline constexpr int count=sizeof(profiles)/sizeof(profiles[0]);
}
