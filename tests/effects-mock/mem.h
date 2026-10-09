#pragma once
#include "common.h"
#include <unordered_map>
#include <type_traits>
namespace mem {
inline std::unordered_map<uintptr_t,unsigned char> bytes;
inline uintptr_t tornParity=0;inline int parityReads=0;
inline bool ValidPtr(uintptr_t p){return p>=0x100000000ULL&&p<0x7FFFFFFFFFFFULL&&(p&7)==0;}
template<class T> inline bool Read(uintptr_t a,T& out){
    if constexpr(std::is_integral_v<T>)if(a==tornParity&&++parityReads>1){out=(T)99;return true;}
    for(size_t i=0;i<sizeof(T);++i)if(!bytes.count(a+i))return false;
    for(size_t i=0;i<sizeof(T);++i)((unsigned char*)&out)[i]=bytes[a+i];return true;
}
template<class T>inline T ReadOr(uintptr_t a,T d=T{}){T v{};return Read(a,v)?v:d;}
template<class T>inline void Put(uintptr_t a,T v){for(size_t i=0;i<sizeof(T);++i)bytes[a+i]=((unsigned char*)&v)[i];}
template<class T>inline bool Write(uintptr_t a,const T& value){Put(a,value);return true;}
inline bool ReadStr(uintptr_t a,char* out,int cap){for(int i=0;i<cap-1;++i){char c=0;if(!Read(a+i,c))return false;out[i]=c;if(!c)return i>0;}out[cap-1]=0;return true;}
inline void PutString(uintptr_t a,const char* s){for(size_t i=0;i<=strlen(s);++i)Put(a+i,s[i]);}
}
namespace rtti { inline std::unordered_map<uintptr_t,const char*> classes;inline const char* ClassOf(uintptr_t,uintptr_t v){auto it=classes.find(v);return it==classes.end()?nullptr:it->second;} }
namespace mem {inline uintptr_t ModuleBase(const char*){return 0;}inline uint32_t ModuleSize(uintptr_t){return 0;}}
namespace rtti {inline bool Ready(){return true;}inline void DetectDelta(uintptr_t,const uintptr_t*,int){}}
