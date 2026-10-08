#pragma once
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Fallback starts at FIRST OBSERVATION, not a proven pickup event. Mark it ~.
namespace aegis {
struct Result { bool visible=false, estimated=false; float seconds=0.f; };
class Tracker {
    struct State {
        uintptr_t item=0; uint32_t serial=0; float end=0;
        bool estimated=true, closed=false;
    };
    std::unordered_map<uintptr_t,State> entries;
    float lastClock=-1;
public:
    void Reset(){entries.clear();lastClock=-1;}
    void BeginFrame(float now){
        if(!std::isfinite(now))return;
        if(lastClock>=0 && now<lastClock-.25f)entries.clear();
        lastClock=now; // simulation time: pausing does not consume the timer
    }
    Result Observe(uintptr_t owner,uintptr_t item,uint32_t serial,bool present,
                   bool absenceConfirmed,float expiresAt,float now,bool alive=true){
        if(!owner||!std::isfinite(now))return {};
        auto it=entries.find(owner);
        if(!present){
            if(it==entries.end())return {};
            // Keep a closed tombstone, so stale inventory data cannot rearm it.
            if(absenceConfirmed||!alive){it->second.closed=true;return {};}
        }else{
            if(it==entries.end()||it->second.item!=item||it->second.serial!=serial){
                if(entries.size()>=256)entries.clear();
                State s;s.item=item;s.serial=serial;s.end=now+300.f;s.closed=!alive;
                it=entries.insert_or_assign(owner,s).first;
            }
            if(!alive)it->second.closed=true; // passive consumption on death, never a cast
            if(it->second.closed)return {};
            float left=expiresAt-now;
            if(std::isfinite(expiresAt)&&expiresAt>0 && left<=300.5f && left>=-300.f){
                it->second.end=expiresAt;it->second.estimated=false;
            }
        }
        State& s=it->second;
        if(s.closed)return {};
        float remaining=std::max(0.f,std::min(300.f,s.end-now));
        if(remaining<=0.f)s.closed=true; // same item never gets another five minutes
        return {remaining>0.f,s.estimated||!present,remaining};
    }
};
}
