#include "mem.h"
#include "runtime_rtti.h"
#include <map>
#include <array>
#include <mutex>
#include <atomic>

namespace rtti {

using RttiEntry = Entry;
static std::atomic<unsigned> runtimeAttempts{0},runtimeParsed{0};
unsigned RuntimeAttempts(){return runtimeAttempts.load();}
unsigned RuntimeParsed(){return runtimeParsed.load();}

#include "rtti_client.inc"

static const RttiEntry* kTable = kClientRtti;
static const int kCount = (int)(sizeof(kClientRtti) / sizeof(kClientRtti[0]));

static int g_delta = 0;
static bool g_ready = false;
static uintptr_t g_clientBase = 0;

const char* Lookup(uint32_t rva) {
    int lo = 0, hi = kCount - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        uint32_t v = kTable[mid].rva;
        if (v == rva) return kTable[mid].name;
        if (v < rva) lo = mid + 1;
        else hi = mid - 1;
    }
    return nullptr;
}

static const char* LookupDelta(uintptr_t clientBase, uintptr_t vptr, int delta) {
    if (!clientBase || vptr < clientBase) return nullptr;
    uint64_t r64 = (uint64_t)(vptr - clientBase);
    if (r64 + (uint64_t)(delta < 0 ? -delta : delta) > 0x10000000ULL) return nullptr;
    uint32_t rva = (uint32_t)((int64_t)r64 - delta);
    return Lookup(rva);
}

bool Ready() { return g_ready; }

void DetectDelta(uintptr_t clientBase, const uintptr_t* objects, int count) {
    g_clientBase = clientBase;
    static const int deltas[] = { 0, 8, -8, 16, -16, 4, -4, 24, -24 };
    int best = 0, bestScore = 0;

    for (int d : deltas) {
        int score = 0;
        for (int i = 0; i < count; ++i) {
            if (!objects[i]) continue;
            if (LookupDelta(clientBase, objects[i], d)) ++score;
        }
        if (score > bestScore) { bestScore = score; best = d; }
    }

    g_delta = best;
    g_ready = bestScore >= 2;
}

const char* ClassOf(uintptr_t clientBase, uintptr_t vptr) {
    if (!vptr) return nullptr;
    // Use current in-memory MSVC RTTI before the old RVA catalog. No engine calls or file reads.
    static std::mutex runtimeMutex;std::lock_guard<std::mutex> runtimeLock(runtimeMutex);
    static uintptr_t runtimeBase=0;static uint32_t runtimeSize=0;
    static std::map<uintptr_t,std::array<char,128>> runtimeNames;
    if(runtimeBase!=clientBase){runtimeBase=clientBase;runtimeSize=mem::ModuleSize(clientBase);runtimeNames.clear();runtimeAttempts.store(0);runtimeParsed.store(0);}
    if(!runtimeSize)runtimeSize=mem::ModuleSize(clientBase);
    auto found=runtimeNames.find(vptr);if(found!=runtimeNames.end())return found->second.data();
    struct Reader {bool Read(uintptr_t p,uintptr_t& out){return mem::Read(p,out);}bool Read(uintptr_t p,runtimertti::Locator& out){return mem::Read(p,out);}bool Read(uintptr_t p,char& out){return mem::Read(p,out);}} reader;
    runtimeAttempts.fetch_add(1);
    std::array<char,128> name{};
    if(runtimertti::Name(reader,clientBase,runtimeSize,vptr,name.data(),name.size())){
        runtimeParsed.fetch_add(1);
        if(runtimeNames.size()<4096)return runtimeNames.emplace(vptr,name).first->second.data();
        // Never return a pointer to stack data when the cache limit is reached.
    }

    if (g_ready) {
        const char* n = LookupDelta(clientBase, vptr, g_delta);
        if (n) return n;
    }
    if (!clientBase) return nullptr;

    static const int deltas[] = { 0, 8, -8, 16, -16, 4, -4 };
    for (int d : deltas) {
        const char* n = LookupDelta(clientBase, vptr, d);
        if (n) return n;
    }
    return nullptr;
}

}
