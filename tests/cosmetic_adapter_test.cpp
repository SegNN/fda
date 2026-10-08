// This checks the real production adapter against a mock Steam interface, NOT a live game.
#include <Windows.h>
#include "../src/skin_changer.h"
#include <cassert>
#include <iostream>
using namespace cosmetic;
static constexpr uint64_t account=0x0110000100004321ULL;
struct GC {void** vt;std::deque<Packet> received;std::vector<Packet> sent;};
int OS(void* self,uint32_t t,const void* p,uint32_t n){auto& gc=*static_cast<GC*>(self);Bytes b;if(n)b.assign(static_cast<const uint8_t*>(p),static_cast<const uint8_t*>(p)+n);gc.sent.push_back({t,std::move(b)});return 0;}
bool OA(void* self,uint32_t* n){auto& gc=*static_cast<GC*>(self);if(gc.received.empty())return false;*n=uint32_t(gc.received.front().data.size());return true;}
int OR(void* self,uint32_t* type,void* destination,uint32_t cap,uint32_t* n){auto& gc=*static_cast<GC*>(self);if(gc.received.empty())return 1;auto& p=gc.received.front();*type=p.type;*n=uint32_t(p.data.size());if(cap<*n)return 2;memcpy(destination,p.data.data(),p.data.size());gc.received.pop_front();return 0;}
static void* original[]={reinterpret_cast<void*>(OS),reinterpret_cast<void*>(OA),reinterpret_cast<void*>(OR)};
static GC gc{original,{},{}};
static int root;
void* Create(const char*){return &root;}int UserHandle(){return 1;}int PipeHandle(){return 2;}void* Generic(void*,int,int,const char*){return &gc;}void* User(void*,int,int,const char*){return &root;}uint64_t SteamID(void*){return account;}
HMODULE GetModuleHandleW(const wchar_t*){return &root;}
void* GetProcAddress(HMODULE,const char* name){
#define EXPORTED(s,f) if(strcmp(name,s)==0)return reinterpret_cast<void*>(f)
 EXPORTED("SteamInternal_CreateInterface",Create);EXPORTED("SteamAPI_GetHSteamUser",UserHandle);EXPORTED("SteamAPI_GetHSteamPipe",PipeHandle);EXPORTED("SteamAPI_ISteamClient_GetISteamGenericInterface",Generic);EXPORTED("SteamAPI_ISteamClient_GetISteamUser",User);EXPORTED("SteamAPI_ISteamUser_GetSteamID",SteamID);return nullptr;
#undef EXPORTED
}
using Send=int(*)(void*,uint32_t,const void*,uint32_t);using Available=bool(*)(void*,uint32_t*);using Retrieve=int(*)(void*,uint32_t*,void*,uint32_t,uint32_t*);
static Packet Read(){uint32_t n=0;assert(reinterpret_cast<Available>(gc.vt[1])(&gc,&n));Bytes b(n);uint32_t type=0,actual=0;assert(reinterpret_cast<Retrieve>(gc.vt[2])(&gc,&type,b.data(),n,&actual)==0);assert(actual==n);return {type,std::move(b)};}
static void Drain(){uint32_t n;for(int i=0;i<100&&reinterpret_cast<Available>(gc.vt[1])(&gc,&n);++i)Read();}
int main(){skins::Poll();assert(skins::GetStatus().connected);assert(gc.vt!=original);assert(skins::SetEnabled(true));assert(gc.sent.size()==1);auto refresh=pb::Decode(Unpack(gc.sent[0].type,gc.sent[0].data).body);assert(pb::GetBytes(refresh,2));
size_t count;auto defs=skins::Catalog(count);const Definition* d=nullptr;for(size_t i=0;i<count;++i)if(defs[i].cls==14&&defs[i].slot==0){d=&defs[i];break;}assert(d);
Bytes owner=pb::Encode({pb::V(1,1),pb::V(2,account)});auto real=pb::Encode({pb::V(1,8080),pb::V(2,uint32_t(account)),pb::V(4,defs[0].id)});auto subscribed=pb::Encode({pb::B(2,pb::Encode({pb::V(1,1),pb::B(2,real)})),pb::F64(3,123),pb::B(4,owner)});auto initial=Pack(24,subscribed);gc.received.push_back(initial);
uint32_t n=0;assert(reinterpret_cast<Available>(gc.vt[1])(&gc,&n));assert(n>initial.data.size());uint32_t type=0,size=0;uint8_t tiny=0;assert(reinterpret_cast<Retrieve>(gc.vt[2])(&gc,&type,&tiny,1,&size)==2);assert(size==n);assert(skins::Select(d->id,d->cls,d->slot,0));auto expanded=Read();assert((expanded.type&~ProtoFlag)==24);assert(expanded.data.size()==n);Drain();assert(skins::GetStatus().cacheReady);assert(skins::GetStatus().definitions==13862);
assert(skins::Select(d->id,d->cls,d->slot,0));auto op=Pack(2569,pb::Encode({pb::B(1,pb::Encode({pb::V(1,FakeId(d->id)),pb::V(2,d->cls),pb::V(3,d->slot)}))}));size_t before=gc.sent.size();assert(reinterpret_cast<Send>(gc.vt[0])(&gc,op.type,op.data.data(),uint32_t(op.data.size()))==0);assert(gc.sent.size()==before);
auto fakeDelete=Pack(1004,pb::Encode({pb::V(1,FakeId(d->id))}));assert(reinterpret_cast<Send>(gc.vt[0])(&gc,fakeDelete.type,fakeDelete.data.data(),uint32_t(fakeDelete.data.size()))==4);assert(gc.sent.size()==before);
auto normal=Pack(9999,pb::Encode({pb::V(1,17)}));assert(reinterpret_cast<Send>(gc.vt[0])(&gc,normal.type,normal.data.data(),uint32_t(normal.data.size()))==0);assert(gc.sent.size()==before+1);
assert(!skins::CanUnload());Drain();assert(skins::SetEnabled(false));assert(!skins::CanUnload());Drain();assert(skins::CanUnload());skins::Shutdown();assert(gc.vt==original);
std::cout<<"Adapter passed: real catalog, mock vtable hookup, buffer retry, local equip interception, no synthetic IDs forwarded, restore and detach.\n";
}
