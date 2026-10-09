#include "rtti.cpp"
#include <cassert>
#include <thread>
#include <iostream>
int main(){uintptr_t base=0x700000000ULL,v=base+0x4944B70,c=base+0x2000;mem::Put(v-8,c);runtimertti::Locator locator{1,0,0,0x4000,0x5000,0x2000};mem::Put(c,locator);mem::String(base+0x4010,".?AVC_DOTAGamerules@@");assert(!strcmp(rtti::ClassOf(base,v),"C_DOTAGamerules"));uintptr_t second=base+0x6000,c2=base+0x2100;mem::Put(second-8,c2);locator.selfRva=0x2100;locator.typeRva=0x4100;mem::Put(c2,locator);mem::String(base+0x4110,".?AVC_DOTAPlayerController@@");assert(!strcmp(rtti::ClassOf(base,second),"C_DOTAPlayerController"));std::thread threads[4];for(auto& t:threads)t=std::thread([&]{for(int i=0;i<1000;++i)assert(!strcmp(rtti::ClassOf(base,second),"C_DOTAPlayerController"));});for(auto& t:threads)t.join();std::cout<<"PASS actual rtti.cpp MOCK integration: current RTTI overrides stale controller RVA, relocated controller is identified, concurrent cache access. No live Windows test.\n";}
