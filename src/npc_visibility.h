#pragma once
#include <cstdint>
namespace npcvisibility {
struct Result {bool known=false,visible=false;uint64_t word=0,selfWord=0;uint32_t index=0;};
// A uint64[256] is 16384 NPC visibility bits, not a team bitmask.
// The adapter must validate current team-data identity/local ownership before/after.
template<class R>Result Read(R& r,uintptr_t bits,uint32_t target,uint32_t self,uint32_t mask){
 Result out;if(!bits||!target||!self||target==0xffffffffu||self==0xffffffffu||mask!=0x3fffu)return out;
 uint32_t index=target&mask,local=self&mask;if(index>=16384||local>=16384)return out;
 uint64_t own=0,own2=0,word=0,word2=0;
 if(!r.Read(bits+8*(local/64),own)||!(own&(uint64_t(1)<<(local%64)))||
    !r.Read(bits+8*(index/64),word)||!r.Read(bits+8*(index/64),word2)||word!=word2||
    !r.Read(bits+8*(local/64),own2)||own2!=own)return out;
 out.known=true;out.visible=(word&(uint64_t(1)<<(index%64)))!=0;out.word=word;out.selfWord=own;out.index=index;return out;
}
}
