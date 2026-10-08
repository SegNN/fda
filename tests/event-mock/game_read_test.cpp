#include "game.cpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace game;
 uintptr_t base=0x100000000ULL,page=0x200000000ULL,e=0x300000000ULL,node=0x400000000ULL;
 g_sys.ready=true;g_sys.clientBase=base;g_sys.imageSize=0x10000000;g_sys.idPages[0]=page;
 uint32_t h=0x8001;uintptr_t identity=page+off::idStride;
 mem::Put(identity,e);mem::Put(identity+off::idHandleFld,h);mem::Put(e+off::instEntity,identity);
 assert(EntityByHandle(h)==e);assert(EntityByHandle(h+0x4000)==0);assert(EntityByIndex(1)==e);
 mem::Put(e+off::BaseEntity::m_pGameSceneNode,node);mem::Put(node+off::SceneNode::m_vecAbsOrigin,Vec3{10,20,30});
 uintptr_t vp=base+0x4907568;mem::Put(e,vp);rtti::classes[vp]="C_DOTA_Item_Rune";
 mem::Put(e+off::Rune::type,7);mem::Put(e+off::Rune::time,100.f);
 StaticUnit u;assert(Classify(e,vp,u)&&u.kind==UnitKind::Rune);
 Frame f;f.now=101;FrameUnit out;assert(FillUnit(u,out,f)&&out.runeRead&&out.runeType==7&&out.entityHandle==h);
 // Rune intentionally has no health fields. NPC-only reads would reject it.
 mem::Put(e+off::Rune::type,999);assert(!FillUnit(u,out,f));mem::Put(e+off::Rune::type,7);
 mem::Put(e+off::Rune::time,110.f);assert(FillUnit(u,out,f)); // Optional time is not a spawn claim.
 rtti::classes[vp]="C_DOTA_Item_RuneSpawner_Powerup";assert(Classify(e,vp,u)&&u.kind==UnitKind::RuneSpawner);
 mem::Put(e+off::RuneSpawner::type,-1);mem::Put(e+off::RuneSpawner::last,100.f);mem::Put(e+off::RuneSpawner::next,120.f);mem::Put(e+off::RuneSpawner::water,(uint8_t)1);
 assert(FillUnit(u,out,f)&&out.nextRuneType==7);
 mem::Put(e+off::RuneSpawner::water,(uint8_t)0);assert(FillUnit(u,out,f)&&out.nextRuneType==-1);
 mem::Put(e+off::RuneSpawner::water,(uint8_t)2);assert(!FillUnit(u,out,f));
 mem::Put(e+off::RuneSpawner::water,(uint8_t)0);mem::Put(identity+off::idHandleFld,h+1);assert(!FillUnit(u,out,f));
 mem::Put(identity+off::idHandleFld,h);
 u.kind=UnitKind::Creep;u.addr=e;u.cls="C_DOTA_Unit_Creep";
 mem::Put(e+off::BaseEntity::m_iHealth,60);mem::Put(e+off::BaseEntity::m_iMaxHealth,100);
 mem::Put(e+off::BaseEntity::m_iTeamNum,(uint8_t)3);mem::Put(e+off::BaseEntity::m_lifeState,(uint8_t)0);
 mem::Put(e+off::NPC::m_flPhysicalArmorValue,0.f);
 f.localAlive=true;f.localTeam=2;f.atkRange=100;f.damageRead=true;f.damageMin=50;f.damageMax=70;f.damageBonus=0;
 cfg::lastHitConservative=true;assert(FillUnit(u,out,f)&&!out.canLastHit);
 cfg::lastHitConservative=false;assert(FillUnit(u,out,f)&&out.canLastHit);
 f.damageRead=false;assert(FillUnit(u,out,f)&&!out.canLastHit);
 uintptr_t manager=e+0xE30,list=0x500000000ULL,buff=0x600000000ULL,name=0x700000000ULL,buffVp=base+0x4577540;
 for(int i=0;i<40;++i)mem::Put(manager+i,uint8_t(0));
 mem::Put(manager+16,1);mem::Put(manager+24,list);mem::Put(list,buff);mem::Put(buff,buffVp);
 rtti::classes[buffVp]="CDOTA_Modifier_FlaskHealing";mem::Put(buff+0x28,name);mem::PutString(name,"modifier_flask_healing");
 mem::Put(buff+0x74,h);mem::Put(buff+0x48,10);mem::Put(buff+0x50,1);mem::Put(buff+0x54,100.f);mem::Put(buff+0x60,10.f);mem::Put(buff+0x64,110.f);mem::Put(buff+0x84,0);
 FrameUnit owner;owner.entityHandle=h;ReadBuffs(e,owner,101.f);assert(owner.buffsRead&&owner.buffs.size()==1);
 mem::Put(buff+0x74,uint32_t(h+0x4000));owner.buffsRead=false;ReadBuffs(e,owner,101.f);assert(!owner.buffsRead);
 uintptr_t ability=0x800000000ULL,abilityList=0x810000000ULL,abilityIdentity=page+2*off::idStride,abilityVp=base+0x123400;
 uint32_t abilityHandle=0x8002;mem::Put(abilityIdentity,ability);mem::Put(abilityIdentity+off::idHandleFld,abilityHandle);mem::Put(ability+off::instEntity,abilityIdentity);
 mem::Put(e+off::NPC::m_vecAbilities,1);mem::Put(e+off::NPC::m_vecAbilities+8,abilityList);mem::Put(abilityList,abilityHandle);
 mem::Put(ability,abilityVp);rtti::classes[abilityVp]="C_DOTA_Ability_Lina_LagunaBlade";
 mem::Put(ability+off::Ability::m_iLevel,1);mem::Put(ability+off::Ability::m_bHidden,false);mem::Put(ability+off::Ability::m_nAbilityBarType,uint32_t(0));
 mem::Put(ability+off::Ability::m_iManaCost,100);mem::Put(ability+off::Ability::m_fCooldown,0.f);mem::Put(ability+off::Ability::m_flCooldownLength,60.f);
 mem::Put(ability+0x619,uint8_t(1));mem::Put(ability+0x630,0.f);mem::Put(ability+0x634,uint8_t(0));mem::Put(ability+0x654,uint8_t(0));mem::Put(ability+0x655,uint8_t(0));
 AbilityInfo info[16];assert(ReadAbilities(e,info,16)==1&&info[0].automationReady);
 mem::Put(ability+0x655,uint8_t(1));assert(ReadAbilities(e,info,16)==1&&!info[0].automationReady);mem::Put(ability+0x655,uint8_t(0));
 for(int i=0;i<4;++i)mem::bytes.erase(ability+off::Ability::m_iManaCost+i);assert(ReadAbilities(e,info,16)==1&&!info[0].automationReady);
 std::cout<<"PASS actual ReadAbilities: readiness, frozen cooldown and missing mana data guards\n";
 std::cout<<"PASS actual ReadBuffs: verified ownership, Flask name and stale parent serial rejection\n";
 std::cout<<"PASS: actual game.cpp mock-memory reads, non-NPC runes, future/random rune types, exact handle serial and invalid fields\n";
}
