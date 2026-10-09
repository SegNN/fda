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
 assert(info[0].addr==ability&&info[0].entityHandle==abilityHandle&&info[0].phaseRead&&!info[0].phase);
 mem::Put(ability+off::Ability::m_bInAbilityPhase,uint8_t(1));assert(ReadAbilities(e,info,16)==1&&info[0].phaseRead&&info[0].phase&&!info[0].automationReady);
 mem::Put(ability+off::Ability::m_bInAbilityPhase,uint8_t(2));assert(ReadAbilities(e,info,16)==1&&!info[0].phaseRead&&!info[0].automationReady);
 mem::Put(ability+off::Ability::m_bInAbilityPhase,uint8_t(0));
 // Runtime zero maximum: exact Valve-published Huskar fallback. Valid override wins.
 rtti::classes[abilityVp]="C_DOTA_Ability_Huskar_Inner_Fire";
 assert(ReadAbilities(e,info,16)==1&&info[0].maxLevel==4);
 mem::Put(ability+off::Ability::m_nMaxLevelOverride,6);assert(ReadAbilities(e,info,16)==1&&info[0].maxLevel==6);
 mem::Put(ability+off::Ability::m_nMaxLevelOverride,0);rtti::classes[abilityVp]="C_DOTA_Ability_Lina_LagunaBlade";

 mem::Put(ability+0x655,uint8_t(1));assert(ReadAbilities(e,info,16)==1&&!info[0].automationReady);mem::Put(ability+0x655,uint8_t(0));
 for(int i=0;i<4;++i)mem::bytes.erase(ability+off::Ability::m_iManaCost+i);assert(ReadAbilities(e,info,16)==1&&!info[0].automationReady);
 // Regression: an early unknown class/name must be retried, not cached forever.
 g_cache.clear();rtti::classes.erase(vp);
 StaticUnit cached;assert(!CachedClassify(e,vp,cached));assert(g_cache.count(e)==0);
 uintptr_t unitName=0x820000000ULL;
 mem::Put(e+off::NPC::m_iszUnitName,unitName);mem::PutString(unitName,"npc_dota_creep_melee");
 assert(CachedClassify(e,vp,cached)&&cached.kind==UnitKind::Creep&&cached.entityHandle==h);
 StaticUnit oldSnapshot=cached;
 // Same address AND vtable, but a new generation and a different unit.
 uint32_t recycled=h+0x4000;mem::Put(identity+off::idHandleFld,recycled);
 mem::PutString(unitName,"npc_dota_roshan");
 assert(CachedClassify(e,vp,cached)&&cached.kind==UnitKind::Roshan&&cached.entityHandle==recycled);
 assert(!FillUnit(oldSnapshot,out,f));
 // A newly resolved RTTI class also invalidates a positive cache entry.
 rtti::classes[vp]="C_DOTA_Unit_Hero_Lina";mem::PutString(unitName,"npc_dota_hero_lina");
 assert(CachedClassify(e,vp,cached)&&cached.kind==UnitKind::Hero);
 assert(strcmp(cached.nick,"Lina")==0);
 uintptr_t shiftedCtrl=base+0x123800;
 rtti::classes[shiftedCtrl]="C_DOTAPlayerController";
 assert(IsPlayerControllerVtable(shiftedCtrl));
 rtti::classes[shiftedCtrl]="C_DOTA_Unit_Hero_Lina";assert(!IsPlayerControllerVtable(shiftedCtrl));
 assert(!IsPlayerControllerVtable(base-8));
 std::cout<<"PASS cache retry, address/vtable recycling, stale frame rejection, late RTTI and controller class guards\n";
 // Controller regression: use the validated dedicated global, not only entity enumeration.
 uintptr_t controller=0x900000000ULL,system=0xA00000000ULL;
 g_sys.esys=system;mem::Put(system+off::idPagesOff,page);
 mem::Put(controller,base+off::Ctrl::vtableRva);
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));
 mem::Put(controller+off::Ctrl::m_nPlayerID,int(7));
 mem::Put(controller+off::Ctrl::m_hAssignedHero,recycled);
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(7));
 mem::Put(base+off::dwLocalPlayerController,controller);
 assert(GlobalLocalController()==0&&g_controllerProbe.globalStage.load()==7);
 assert(ChooseLocalController(controller,0,0)==controller&&g_controllerProbe.source.load()==1);
 assert(ChooseLocalController(0,controller,1)==controller&&g_controllerProbe.source.load()==2);
 assert(ChooseLocalController(0,controller,2)==0&&g_controllerProbe.source.load()==3);
 assert(ChooseLocalController(0,0,0)==0);
 // Flag=0 is accepted ONLY from the dedicated global with matching hero ownership.
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(0));
 assert(!ControllerCandidate(controller,false));assert(ControllerCandidate(controller,true));
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(8));assert(!ControllerCandidate(controller,true));
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(7));
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(2));assert(!ControllerCandidate(controller,true));
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(0));
 mem::Put(controller+off::Ctrl::m_hAssignedHero,recycled+0x4000);assert(!ControllerCandidate(controller,true));
 mem::Put(controller+off::Ctrl::m_hAssignedHero,recycled);
 assert(GlobalLocalController()==0);mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));ChooseLocalController(0,controller,1);
 g_units={cached};BuildFrame(f,controller);
 assert(f.ok&&f.localHero==e&&f.localHandle==recycled&&f.units.size()==1&&g_controllerProbe.frameStage.load()==10);
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(8));BuildFrame(f,controller);assert(!f.ok);
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(7));
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));
 mem::Put(controller+off::Ctrl::m_hAssignedHero,uint32_t(0));BuildFrame(f,controller);assert(!f.ok&&g_controllerProbe.frameStage.load()==5);
 mem::Put(controller+off::Ctrl::m_hAssignedHero,recycled);
 mem::Put(node+off::SceneNode::m_vecAbsOrigin,Vec3{NAN,20,30});BuildFrame(f,controller);assert(!f.ok&&g_controllerProbe.frameStage.load()==7);
 mem::Put(node+off::SceneNode::m_vecAbsOrigin,Vec3{10,20,30});
 mem::Put(e+off::BaseEntity::m_iMaxHealth,int(0));BuildFrame(f,controller);assert(!f.ok&&g_controllerProbe.frameStage.load()==8);
 mem::Put(e+off::BaseEntity::m_iMaxHealth,int(100));
 mem::Put(base+off::dwLocalPlayerController,e);assert(GlobalLocalController()==0&&g_controllerProbe.globalStage.load()==7);
 mem::Put(base+off::dwLocalPlayerController,uintptr_t(0));assert(GlobalLocalController()==0&&g_controllerProbe.globalStage.load()==7);
 mem::Put(base+off::dwLocalPlayerController,controller);
 assert(GlobalLocalController()==0);mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));ChooseLocalController(0,controller,1);BuildFrame(f,controller);assert(f.ok);
 // Entity zero must not be skipped: it can carry a valid serial/identity.
 mem::Put(page,controller);mem::Put(page+off::idHandleFld,uint32_t(0x8000));mem::Put(controller+off::instEntity,page);
 assert(EntityByIndex(0)==controller&&EntityByHandle(0x8000)==controller);
 // No local controller: world ESP data survives, without inventing local identity or timers.
 g_sys.localCtrl=0;BuildObservedFrame(f);
 assert(!f.ok&&f.observedOnly&&!f.localAlive&&f.localHero==0&&f.localHandle==0&&f.localTeam==0);
 assert(f.units.size()==1&&f.units[0].kind==UnitKind::Hero&&f.units[0].hp==60);
 assert(f.units[0].abilN==1&&f.units[0].itemN==0&&!f.units[0].canLastHit&&!f.units[0].canDeny);
 mem::Put(identity+off::idHandleFld,recycled+0x4000);BuildObservedFrame(f);assert(f.observedOnly&&!f.ok&&f.units.empty());
 mem::Put(identity+off::idHandleFld,recycled);
 mem::Put(node+off::SceneNode::m_vecAbsOrigin,Vec3{NAN,0,0});BuildObservedFrame(f);assert(f.units.empty());
 mem::Put(node+off::SceneNode::m_vecAbsOrigin,Vec3{10,20,30});BuildObservedFrame(f);assert(f.units.size()==1&&!f.ok);
 std::cout<<"PASS typed controller guards, disabled invalid global, entity zero, observed-only world snapshot, stale handle and invalid-position rejection\n";
 // World visual reads must survive missing local controller without enabling input.
 mem::Put(e+off::BaseEntity::m_flSimulationTime,101.f);mem::Put(e+off::NPC::m_bIsIllusion,uint8_t(1));
 mem::Put(e+off::NPC::m_flInvisibilityLevel,.8f);mem::Put(e+off::NPC::m_nUnitState64,uint64_t(1)<<7);
 mem::Put(buff+0x74,recycled);buffLayouts.clear();
 uintptr_t inventory=e+off::NPC::m_Inventory,itemList=0xA10000000ULL;
 mem::Put(inventory+off::Inventory::m_iParity,int(1));mem::Put(inventory+off::Inventory::m_hItems,int(1));mem::Put(inventory+off::Inventory::m_hItems+8,itemList);mem::Put(itemList,abilityHandle);
 rtti::classes[abilityVp]="C_DOTA_Item_Armlet";mem::Put(ability+off::Item::m_iCurrentCharges,int(0));
 BuildObservedFrame(f);assert(!f.ok&&f.observedOnly&&f.localHandle==0);
 assert(f.units.size()==1&&f.units[0].clockRead&&f.units[0].illusionRead&&f.units[0].illusion&&f.units[0].invisRead&&f.units[0].stateRead);
 assert(f.units[0].buffsRead&&f.units[0].buffs.size()==1&&f.units[0].inventoryRead&&f.units[0].itemN==1&&!strcmp(f.units[0].items[0].icon,"armlet"));
 assert(buffreader::Remaining(f.units[0].buffs[0],f.units[0].sampleTime)==9);
 mem::Put(manager+20,int(1));buffLayouts.clear();BuildObservedFrame(f);assert(!f.units[0].buffsRead&&f.units[0].buffsVisualRead&&f.units[0].buffs.size()==1);
 mem::Put(manager+20,int(0));buffLayouts.clear();
 std::cout<<"PASS HUD5 observed-only item, modifier ownership, per-entity clock, illusion/invis/state reads without local identity or automation.\n";
 // ARM9 alternate vector layout: pointer at +0 / count at +8, schema parent full handle.
 mem::Put(inventory+0xA8,recycled);mem::Put(inventory+off::Inventory::m_hItems,itemList);mem::Put(inventory+off::Inventory::m_hItems+8,int(1));
 FrameUnit alt;alt.entityHandle=recycled;ReadItems(e,alt);assert(alt.inventoryRead&&alt.inventoryLayout==2&&alt.itemN==1&&alt.items[0].instanceHandle==abilityHandle);
 mem::Put(inventory+0xA8,recycled+0x4000);ReadItems(e,alt);assert(!alt.inventoryRead);mem::Put(inventory+0xA8,recycled);
 mem::tornParity=inventory+off::Inventory::m_iParity;mem::parityReads=0;ReadItems(e,alt);assert(!alt.inventoryRead);mem::tornParity=0;
 std::cout<<"PASS ARM9 actual inventory reader: pointer-first layout, exact item serial/backlink, inventory parent ownership and torn parity rejection.\n";
 // ARM10 exact reported inline layout: count 25 followed by 25 32-bit handles, not pointers.
 mem::Put(inventory+off::Inventory::m_hItems,int(25));for(int i=0;i<25;++i)mem::Put(inventory+off::Inventory::m_hItems+4+4*i,uint32_t(0xFFFFFFFFu));
 mem::Put(inventory+off::Inventory::m_hItems+4,abilityHandle);ReadItems(e,alt);
 assert(alt.inventoryRead&&alt.inventoryLayout==3&&alt.itemN==1&&alt.items[0].slot==0&&alt.items[0].instanceHandle==abilityHandle);
 mem::Put(inventory+0xA8,recycled+0x4000);ReadItems(e,alt);assert(!alt.inventoryRead);mem::Put(inventory+0xA8,recycled);
 mem::Put(inventory+off::Inventory::m_hItems+4,uint32_t(abilityHandle+0x4000));ReadItems(e,alt);assert(!alt.inventoryRead);
 mem::Put(inventory+off::Inventory::m_hItems+4,uint32_t(0xFFFFFFFFu));ReadItems(e,alt);assert(alt.inventoryRead&&alt.itemN==0&&alt.inventoryLayout==3);
 // Positive dedicated Hero player ID may prove demo ownership, but conflicts are rejected.
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(-1));mem::Put(e+off::Hero::m_iPlayerID,int(7));assert(HeroBelongsToPlayer(e,7));
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(8));assert(!HeroBelongsToPlayer(e,7));
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(7));mem::Put(e+off::Hero::m_iPlayerID,int(8));assert(!HeroBelongsToPlayer(e,7));
 mem::Put(e+off::Hero::m_iPlayerID,int(7));assert(HeroBelongsToPlayer(e,7));
 std::cout<<"PASS ARM10 inline inventory count25/handles+4, parent/serial rejection and actual empty inventory; dedicated hero owner ID with conflict rejection.\n";
 // A global reference can reach a controller absent from entity enumeration.
 auto prepareRefs=[&](std::initializer_list<uintptr_t> values){g_controllerRefs=ControllerReferenceScan{};auto& scan=g_controllerRefs;scan.base=base;scan.collecting=true;uintptr_t refs=base+0x1000;scan.cursor=refs;scan.ranges={{refs,refs+8*values.size()}};int n=0;for(auto v:values)mem::Put(refs+8*n++,v);};
 assert(StrictLocalReference(controller));prepareRefs({controller,controller});assert(SearchLocalControllerRefs(1)==0);assert(SearchLocalControllerRefs(1)==controller&&g_controllerProbe.controllerRefStage.load()==4);
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(0));assert(!StrictLocalReference(controller));mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));
 mem::Put(e+off::NPC::m_nPlayerOwnerID,int(8));assert(!StrictLocalReference(controller));mem::Put(e+off::NPC::m_nPlayerOwnerID,int(7));
 uintptr_t other=controller+0x1000;mem::Put(other,base+off::Ctrl::vtableRva);mem::Put(other+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));mem::Put(other+off::Ctrl::m_nPlayerID,int(7));mem::Put(other+off::Ctrl::m_hAssignedHero,recycled);
 prepareRefs({controller,other});assert(SearchLocalControllerRefs(2)==0&&g_controllerProbe.controllerRefStage.load()==5);
 prepareRefs({controller});g_controllerRefs.truncated=true;assert(SearchLocalControllerRefs(1)==0&&g_controllerProbe.controllerRefStage.load()==6);
 prepareRefs({base+0x800000});assert(SearchLocalControllerRefs(1)==0&&g_controllerProbe.controllerRefStage.load()==3);
 std::cout<<"PASS HUD5 bounded controller-reference scan: complete-pass uniqueness, duplicate references, non-local/owner mismatch, ambiguous controllers and no-hit rejection.\n";
 // Designer metadata is an independent, serial-verified NPC name fallback.
 g_cache.clear();rtti::classes.erase(vp);
 uintptr_t emptyName=0x830000000ULL;mem::Put(e+off::NPC::m_iszUnitName,emptyName);
 mem::PutString(emptyName,"");mem::Put(identity+off::Identity::m_designerName,unitName+3);
 mem::PutString(unitName+3,"npc_dota_hero_tidehunter");
 StaticUnit designer;assert(CachedClassify(e,vp,designer)&&designer.kind==UnitKind::Hero);
 assert(strcmp(designer.name,"npc_dota_hero_tidehunter")==0);
 mem::PutString(unitName+3,"C_DOTAPlayerController");char rejectedName[64];assert(!ReadDesignerUnitName(e,rejectedName,64));
 // Announcer names share hero prefix; reject even when RTTI claims a hero.
 g_cache.clear();mem::PutString(emptyName,"npc_dota_hero_announcer");StaticUnit announcer;
 assert(!Classify(e,vp,announcer)&&!IsHeroEntity(e));
 mem::PutString(emptyName,"npc_dota_hero_announcer_killing_spree");assert(!Classify(e,vp,announcer));
 rtti::classes[vp]="C_DOTA_Unit_Hero_Announcer";assert(!Classify(e,vp,announcer)&&!IsHeroEntity(e));
 rtti::classes[vp]="C_DOTA_Unit_Hero_TemplarAssassin";mem::PutString(emptyName,"npc_dota_hero_templar_assassin");
 assert(Classify(e,vp,announcer)&&announcer.kind==UnitKind::Hero&&IsHeroEntity(e));
 std::cout<<"PASS ESP4 excludes announcer/killing-spree pseudo-heroes and retains real heroes.\n";
 // ESP19 actual guarded table adapter: zero model mask may coexist with a visible NPC table bit.
 uintptr_t data=0xb10000000ULL,dataVp=base+0x123a00;uint32_t dataHandle=0x8014;uintptr_t dataId=page+20*off::idStride;
 mem::Put(dataId,data);mem::Put(dataId+off::idHandleFld,dataHandle);mem::Put(data+off::instEntity,dataId);mem::Put(data,dataVp);rtti::classes[dataVp]="C_DOTA_DataRadiant";
 uintptr_t foe=0xb20000000ULL;uint32_t foeHandle=0x801e;uintptr_t foeId=page+30*off::idStride;
 mem::Put(foeId,foe);mem::Put(foeId+off::idHandleFld,foeHandle);mem::Put(foe+off::instEntity,foeId);
 g_teamVisibilityData[0].store(data);g_teamVisibilityDataHandle[0].store(dataHandle);g_sys.localCtrl=controller;mem::Put(controller+off::Ctrl::m_hAssignedHero,recycled);mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));
 Frame visibleFrame;visibleFrame.ok=true;visibleFrame.localHero=e;visibleFrame.localHandle=recycled;visibleFrame.localTeam=2;FrameUnit foeUnit;foeUnit.addr=foe;foeUnit.entityHandle=foeHandle;
 mem::Put(data+off::TeamVisibilityData::m_bNPCVisibleState,uint64_t((1ULL<<1)|(1ULL<<30)));ReadNPCVisibility(foeUnit,visibleFrame);assert(foeUnit.npcVisibilityRead&&foeUnit.npcVisible);
 mem::Put(data+off::TeamVisibilityData::m_bNPCVisibleState,uint64_t(1ULL<<1));foeUnit.npcVisibilityRead=false;ReadNPCVisibility(foeUnit,visibleFrame);assert(foeUnit.npcVisibilityRead&&!foeUnit.npcVisible);
 rtti::classes[dataVp]="C_DOTA_DataSpectator";foeUnit.npcVisibilityRead=false;ReadNPCVisibility(foeUnit,visibleFrame);assert(!foeUnit.npcVisibilityRead);rtti::classes[dataVp]="C_DOTA_DataRadiant";
 mem::Put(dataId+off::idHandleFld,uint32_t(dataHandle+0x4000));ReadNPCVisibility(foeUnit,visibleFrame);assert(!foeUnit.npcVisibilityRead);mem::Put(dataId+off::idHandleFld,dataHandle);
 mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(0));ReadNPCVisibility(foeUnit,visibleFrame);assert(!foeUnit.npcVisibilityRead);mem::Put(controller+off::Ctrl::m_bIsLocalPlayerController,uint8_t(1));
 mem::Put(data+off::TeamVisibilityData::m_bNPCVisibleState,uint64_t(1ULL<<30));ReadNPCVisibility(foeUnit,visibleFrame);assert(!foeUnit.npcVisibilityRead); // own bit missing
 visibleFrame.localTeam=3;ReadNPCVisibility(foeUnit,visibleFrame);assert(!foeUnit.npcVisibilityRead);g_teamVisibilityData[0].store(0);
 std::cout<<"PASS actual team NPC table reader: zero model mask, visible->hidden, self bit, team/type/spectator/serial/local-flag rejection. Static source candidate, NOT live offset validation.\n";
 std::cout<<"PASS strict designer-name fallback, back-reference and full serial checks\n";
 std::cout<<"PASS actual ReadAbilities: readiness, frozen cooldown and missing mana data guards\n";
 std::cout<<"PASS actual ReadBuffs: verified ownership, Flask name and stale parent serial rejection\n";
 std::cout<<"PASS: actual game.cpp mock-memory reads, non-NPC runes, future/random rune types, exact handle serial and invalid fields\n";
}
