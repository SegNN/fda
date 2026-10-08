#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Portable, read-only event state. No Windows, memory writes, or hardcoded rune schedules.
namespace mapeventcore {
enum class Kind { Rune, Spawner, EnemyWard, Roshan };
enum class EventKind { RuneDiscovered, RuneSpawnObserved, RuneSoon, WardDiscovered, RoshanAlive, RoshanDead };
struct Observation {
    uint32_t handle=0; Kind kind=Kind::Rune; int type=-1, nextType=-1;
    float stamp=-1, next=-1, x=0, y=0; bool alive=true;
};
struct Event { EventKind kind; int type; float seconds, x, y; uint32_t handle; };
struct Settings { bool runes=true, soon=true, wards=false, roshan=true; float lead=10; };
class Tracker {
    struct State { Observation last; float reminded=-1; };
    std::unordered_map<uint64_t,State> seen;
    uint64_t session=0; float start=-1, previous=-1;
    bool initialized=false; Settings old{};
    static bool Valid(const Observation& o,float now) {
        if(!o.handle || !std::isfinite(o.x)||!std::isfinite(o.y) || std::abs(o.x)>50000 || std::abs(o.y)>50000)return false;
        if(o.kind==Kind::Rune || o.kind==Kind::Spawner) {
            if(o.type < -1 || o.type>9 || !std::isfinite(o.stamp) || o.stamp < -120 || o.stamp>now+1)return false;
            if(o.kind==Kind::Rune && o.type<0)return false;
            if(o.kind==Kind::Spawner && (!std::isfinite(o.next)||o.next<-1||o.next>now+3600 || o.nextType < -1 || o.nextType>9))return false;
        }
        return true;
    }
public:
    void Reset(){seen.clear();initialized=false;previous=-1;}
    std::vector<Event> Update(bool valid,uint64_t match,float gameStart,float now,const std::vector<Observation>& observations,const Settings& cfg) {
        std::vector<Event> result;
        if(!valid || !match || !std::isfinite(now)||!std::isfinite(gameStart) || now < -120 || now>86400) {Reset();return result;}
        // Reconnect, match replacement, clock rollback or a long lost-frame gap: rebaseline.
        bool baseline=!initialized || session!=match || start!=gameStart || now < previous-.25f || now-previous>5.f;
        if(baseline)seen.clear();
        session=match;start=gameStart;previous=now;
        bool runeBaseline=baseline || (cfg.runes&&!old.runes);
        bool soonBaseline=baseline || (cfg.soon&&!old.soon) || cfg.lead!=old.lead;
        bool wardBaseline=baseline || (cfg.wards&&!old.wards);
        bool roshanBaseline=baseline || (cfg.roshan&&!old.roshan);
        float lead=std::isfinite(cfg.lead)?std::clamp(cfg.lead,3.f,30.f):10.f;
        auto emit=[&](EventKind k,const Observation& o,int type,float seconds=0){if(result.size()<16)result.push_back({k,type,seconds,o.x,o.y,o.handle});};
        for(const auto& o:observations) {
            if(!Valid(o,now))continue;
            uint64_t key=((uint64_t)o.kind<<32)|o.handle;
            auto it=seen.find(key);
            if(it==seen.end()) {
                seen.emplace(key,State{o,(o.kind==Kind::Spawner && o.next>now && o.next-now<=lead)?o.next:-1});
                if(o.kind==Kind::Rune && cfg.runes&&!runeBaseline)emit(EventKind::RuneDiscovered,o,o.type);
                if(o.kind==Kind::EnemyWard && o.alive && cfg.wards&&!wardBaseline)emit(EventKind::WardDiscovered,o,-1);
                // Never call first observation "spawned" or a missing Roshan "dead".
                continue;
            }
            auto& s=it->second;
            if(o.kind==Kind::Spawner) {
                if(cfg.runes&&!runeBaseline && o.stamp>s.last.stamp+.1f && now-o.stamp<=2.f)
                    emit(EventKind::RuneSpawnObserved,o,-1); // Random type may refer to previous rune.
                float remaining=o.next-now;
                if(cfg.soon&&!soonBaseline && o.next>0 && o.next==s.last.next && s.last.next>0 &&
                   remaining>0 && remaining<=lead &&
                   s.reminded!=o.next) {
                    // Missing/late discovery still gives a future-only reminder, never a spawn confirmation.
                    emit(EventKind::RuneSoon,o,o.nextType,remaining);s.reminded=o.next;
                }
                if(soonBaseline && remaining>0 && remaining<=lead)s.reminded=o.next;
            }
            if(o.kind==Kind::Rune && cfg.runes&&!runeBaseline && o.type!=s.last.type)
                emit(EventKind::RuneDiscovered,o,o.type);
            if(o.kind==Kind::Roshan && cfg.roshan&&!roshanBaseline && o.alive!=s.last.alive)
                emit(o.alive?EventKind::RoshanAlive:EventKind::RoshanDead,o,-1);
            s.last=o;
        }
        const auto rawEvents=result;
        result.erase(std::remove_if(result.begin(),result.end(),[&](const Event& e){
            if(e.kind!=EventKind::RuneDiscovered)return false;
            for(const auto& other:rawEvents)if(other.kind==EventKind::RuneSpawnObserved &&
                std::hypot(e.x-other.x,e.y-other.y)<250.f)return true;
            return false;
        }),result.end());
        // Keep disappeared handles for deduplication across visibility flicker. Bound memory.
        if(seen.size()>8192)Reset();else initialized=true;
        old=cfg;return result;
    }
};
}
