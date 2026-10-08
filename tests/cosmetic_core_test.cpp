#include "../src/cosmetic_core.h"
#include <cassert>
#include <iostream>
#include <random>
using namespace cosmetic;
static Definition defs[]={{100,14,0,4,"0,1","Test weapon","Pudge","weapon","immortal"},{101,14,1,4,"0","Test head","Pudge","head","rare"},{102,0,14,4,"0,2","Test terrain","Global","terrain","mythical"},{100,15,2,4,"0,1","Test weapon","Razor","weapon","immortal"}};
static constexpr uint64_t account=0x0110000100001234ULL;
static Bytes owner(uint64_t id=account){return pb::Encode({pb::V(1,1),pb::V(2,id)});}
static Bytes real(uint64_t id,uint32_t def,uint32_t cls,uint32_t slot){return pb::Encode({pb::V(1,id),pb::V(2,uint32_t(account)),pb::V(4,def),pb::B(18,pb::Encode({pb::V(1,cls),pb::V(2,slot)})),pb::B(99,{1,2,3})});}
static Packet cache(uint64_t who=account){auto type=pb::Encode({pb::V(1,1),pb::B(2,real(400,300,14,0)),pb::B(2,real(401,301,14,1)),pb::B(2,real(402,302,0,14)),pb::V(77,123)});return Pack(24,pb::Encode({pb::B(2,type),pb::F64(3,500),pb::B(4,owner(who)),pb::V(88,999)}));}
static Packet equip(uint64_t id,uint32_t cls,uint32_t slot,uint32_t style=255,uint32_t type=1059){auto m=pb::Encode({pb::V(1,id),pb::V(2,cls),pb::V(3,slot),pb::V(4,style)});return Pack(type,type==2569?pb::Encode({pb::B(1,m)}):m,pb::Encode({pb::F64(10,42)}));}
static void drain(Core& c){Packet p;while(c.Pop(p)){};}
static pb::Message itemFromCache(const Packet& p,uint64_t id){auto m=pb::Decode(Unpack(p.type,p.data).body);for(const auto& f:m)if(f.n==2){auto t=pb::Decode(f.bytes);if(pb::Get(t,1)==1)for(const auto& x:t)if(x.n==2){auto i=pb::Decode(x.bytes);if(pb::Get(i,1)==id)return i;}}return {};}
int main(){Core c(defs,4);c.SetOwner(account);assert(c.Count()==3);auto incoming=cache();auto original=incoming.data;c.Enable(true);assert(!c.Ready());auto out=c.Incoming(incoming.type,incoming.data);assert(c.Ready());assert(incoming.data==original);assert(pb::Get(itemFromCache(out,FakeId(100)),4)==100);assert(pb::Get(itemFromCache(out,400),1)==400);assert(pb::Get(pb::Decode(Unpack(out.type,out.data).body),88)==999);
assert(c.Select(100,14,0,1));assert(c.Selected()==1);drain(c);out=c.Incoming(incoming.type,incoming.data);auto fake=itemFromCache(out,FakeId(100));assert(pb::Get(fake,15)==1);assert(pb::GetBytes(fake,18));auto actual=itemFromCache(out,400);assert(!pb::GetBytes(actual,18));assert(pb::GetBytes(actual,99));assert(pb::GetBytes(itemFromCache(out,401),18));
assert(!c.Select(100,14,1,0));assert(!c.Select(100,14,0,2));assert(c.Selected()==1);
auto invalid=equip(FakeId(100),14,1);assert(c.Outgoing(invalid.type,invalid.data)==4);auto valid=equip(FakeId(101),14,1,0,2569);assert(c.Outgoing(valid.type,valid.data)==0);assert(c.Selected()==2);Packet reply;bool found=false;while(c.Pop(reply))if((reply.type&~ProtoFlag)==2570){auto h=pb::Decode(Unpack(reply.type,reply.data).header);assert(pb::Get(h,11)==42);found=true;}assert(found);
auto style=Pack(2577,pb::Encode({pb::V(1,FakeId(100)),pb::V(2,1)}));assert(c.Outgoing(style.type,style.data)==0);drain(c);auto badStyle=Pack(2577,pb::Encode({pb::V(1,FakeId(100)),pb::V(2,9)}));assert(c.Outgoing(badStyle.type,badStyle.data)==4);assert(c.Choices().at({14,0}).style==1);
auto other=Pack(9999,pb::Encode({pb::V(1,7)}));assert(c.Outgoing(other.type,other.data)==-1);auto leak=Pack(9999,pb::Encode({pb::B(2,pb::Encode({pb::V(9,FakeId(100))}))}));assert(c.Outgoing(leak.type,leak.data)==4);
auto foreign=cache(account+1);assert(c.Incoming(foreign.type,foreign.data).data==foreign.data);assert(c.Selected()==2);
auto delta=Pack(22,pb::Encode({pb::V(2,1),pb::B(3,real(400,300,14,0)),pb::F64(4,501),pb::B(5,owner())}));auto updated=c.Incoming(delta.type,delta.data);auto um=pb::Decode(Unpack(updated.type,updated.data).body);assert(!pb::GetBytes(pb::Decode(*pb::GetBytes(um,3)),18));
assert(c.Reset());drain(c);assert(c.Selected()==0);assert(pb::GetBytes(itemFromCache(c.Incoming(incoming.type,incoming.data),400),18));c.Select(100,14,0,1);drain(c);c.Enable(false);assert(c.Pop(reply));auto removed=pb::Decode(Unpack(reply.type,reply.data).body);assert(std::count_if(removed.begin(),removed.end(),[](auto& f){return f.n==5;})==3);assert(c.Incoming(incoming.type,incoming.data).data==incoming.data);assert(c.Outgoing(valid.type,valid.data)==4);assert(c.Outgoing(other.type,other.data)==-1);
Core w(defs,4);w.SetOwner(account);auto welcome=Pack(4004,pb::Encode({pb::V(1,123),pb::B(3,Unpack(incoming.type,incoming.data).body),pb::B(9,{5,4,3})}));w.Enable(true);auto wo=w.Incoming(welcome.type,welcome.data);assert(w.Ready());assert(pb::Get(pb::Decode(Unpack(wo.type,wo.data).body),1)==123);auto malformed=welcome;malformed.data[4]=255;assert(w.Incoming(malformed.type,malformed.data).data==malformed.data);assert(w.Outgoing(malformed.type,malformed.data)==4);
auto refresh=pb::Decode(Unpack(w.Refresh().type,w.Refresh().data).body);assert(pb::GetBytes(refresh,2)&&!pb::GetBytes(refresh,1));
// Inventory style can be chosen before the item has been equipped.
Core native(defs,4);native.SetOwner(account);native.Enable(true);native.Incoming(incoming.type,incoming.data);
auto unEquippedStyle=Pack(2577,pb::Encode({pb::V(1,FakeId(100)),pb::V(2,1)}));assert(native.Outgoing(unEquippedStyle.type,unEquippedStyle.data)==0);drain(native);auto noStyleEquip=equip(FakeId(100),14,0);assert(native.Outgoing(noStyleEquip.type,noStyleEquip.data)==0);assert(native.Choices().at({14,0}).style==1);
// An owned definition appears once as the original item, not again as a synthetic copy.
Core dedup(defs,4);dedup.SetOwner(account);dedup.Enable(true);auto ownedCache=Pack(24,pb::Encode({pb::B(2,pb::Encode({pb::V(1,1),pb::B(2,real(999,100,14,0))})),pb::F64(3,500),pb::B(4,owner())}));auto dedupOut=dedup.Incoming(ownedCache.type,ownedCache.data);assert(itemFromCache(dedupOut,FakeId(100)).empty());assert(!itemFromCache(dedupOut,999).empty());assert(!itemFromCache(dedupOut,FakeId(101)).empty());
auto ownedStyle=Pack(2577,pb::Encode({pb::V(1,999),pb::V(2,1)}));assert(dedup.Outgoing(ownedStyle.type,ownedStyle.data)==0);drain(dedup);auto ownedProjected=dedup.Incoming(ownedCache.type,ownedCache.data);assert(pb::Get(itemFromCache(ownedProjected,999),15)==1);assert(dedup.Select(100,14,0,1));assert(dedup.Choices().at({14,0}).id==999);assert(dedup.SelectedDefinitions().at({14,0}).first==100);
// Valid-wire unknown-field round trips and bounded malformed input fuzzing.
for(uint64_t value:{uint64_t(0),uint64_t(127),uint64_t(128),uint64_t(UINT64_MAX)}){auto m=pb::Message{pb::V(7,value),pb::F64(8,value),pb::B(9,{0,1,255})};assert(pb::Encode(pb::Decode(pb::Encode(m)))==pb::Encode(m));}
std::mt19937 random(771);for(int i=0;i<6000;++i){Bytes b(size_t(random()%120));for(auto& x:b)x=uint8_t(random());try{auto m=pb::Decode(b);assert(pb::Encode(pb::Decode(pb::Encode(m)))==pb::Encode(m));}catch(const std::exception&){}auto p=Pack(24,b);(void)w.Incoming(p.type,p.data);}
// A partial batch must not mutate a valid earlier entry.
auto a=Unpack(equip(FakeId(101),14,1,0).type,equip(FakeId(101),14,1,0).data).body;auto b=Unpack(invalid.type,invalid.data).body;auto batch=Pack(2569,pb::Encode({pb::B(1,a),pb::B(1,b)}));auto before=w.Choices();assert(w.Outgoing(batch.type,batch.data)==4);assert(w.Choices().size()==before.size());
w.SetOwner(account+1);assert(!w.Enabled()&&!w.Ready()&&!w.HasQueued()&&w.Selected()==0);
std::cout<<"Cosmetics: protocol, ownership isolation, equip/style, reset, rollback and 6000 malformed-wire cases passed.\n";
}
