#include "kill_helper.cpp"
#include <cassert>
#include <iostream>
namespace clientui {bool English(){return false;}}
int main(){
 Frame f;f.ok=f.localAlive=true;f.localHero=0x100000000ULL;f.queryUnit=0x200000000ULL;f.localTeam=2;f.mana=500;
 FrameUnit self;self.addr=f.localHero;self.kind=UnitKind::Hero;self.team=2;strcpy(self.name,"npc_dota_hero_lina");self.buffsRead=true;self.abilN=2;
 auto& a=self.abil[0];strcpy(a.icon,"lina_laguna_blade");a.slot=5;a.level=1;a.mana=150;a.automationReady=true;
 strcpy(self.abil[1].icon,"lina_flame_cloak");self.abil[1].level=1;self.abil[1].slot=3;self.abil[1].mana=0;self.abil[1].automationReady=true;
 FrameUnit enemy;enemy.addr=f.queryUnit;enemy.kind=UnitKind::Hero;enemy.team=3;enemy.alive=true;enemy.hp=250;enemy.pos={500,0,0};enemy.buffsRead=true;enemy.teamVisibilityRead=true;enemy.teamVisibilityMask=4;strcpy(enemy.nick,"Pudge");
 f.units={self,enemy};mem::Put(self.addr+0x12B8,uint64_t(0));mem::Put(enemy.addr+0x12B8,uint64_t(0));mem::Put(enemy.addr+0x1630,.25f);cfg::helperMargin=30;
 auto report=killhelper::Build(f);assert(report.target&&report.rows.size()==2&&report.rows[0].result.damage==300&&report.rows[0].result.delta==50&&report.rows[0].result.status==killhelpercore::Status::Enough);assert(!report.rows[1].modeled);
 f.queryUnit=0;assert(!killhelper::Build(f).target);f.queryUnit=enemy.addr;
 f.units[1].teamVisibilityMask=0;assert(!killhelper::Build(f).target);f.units[1].teamVisibilityMask=4;
 f.units[1].team=2;assert(!killhelper::Build(f).target);f.units[1].team=3;
 f.units[1].illusion=true;assert(!killhelper::Build(f).target);f.units[1].illusion=false;
 f.units[1].invis=1;assert(!killhelper::Build(f).target);f.units[1].invis=0;
 f.units[1].buffsRead=false;assert(killhelper::Build(f).rows[0].result.status==killhelpercore::Status::Unknown);f.units[1].buffsRead=true;
 f.units[0].abil[0].level=0;assert(!killhelper::Build(f).rows[0].modeled);f.units[0].abil[0].level=1;
 strcpy(cfg::ksHeroName,self.name);cfg::ksAbilitySlot=3;cfg::ksDamage=500;cfg::ksRange=700;cfg::ksDamageType=2;
 report=killhelper::Build(f);bool manual=false;for(auto row:report.rows)if(row.manual){manual=true;assert(row.result.damage==500);}assert(manual);
 strcpy(cfg::ksHeroName,"npc_dota_hero_lion");report=killhelper::Build(f);for(auto row:report.rows)assert(!row.manual);
 assert(winmock::keyInputs==0&&winmock::clicks==0);
 std::cout<<"PASS production helper under mock memory: hover selection, visibility/team/illusion/invis, unknown protection, slot-independent name models, manual hero lock, zero input synthesis\n";
}
