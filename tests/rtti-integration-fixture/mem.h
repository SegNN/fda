#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>
namespace mem {inline std::unordered_map<uintptr_t,unsigned char> bytes;inline uint32_t ModuleSize(uintptr_t){return 0x6000000;}template<class T>bool Read(uintptr_t p,T& out){for(size_t i=0;i<sizeof(T);++i)if(!bytes.count(p+i))return false;for(size_t i=0;i<sizeof(T);++i)((unsigned char*)&out)[i]=bytes[p+i];return true;}template<class T>void Put(uintptr_t p,T v){for(size_t i=0;i<sizeof(T);++i)bytes[p+i]=((unsigned char*)&v)[i];}inline void String(uintptr_t p,const char* s){for(size_t i=0;i<=strlen(s);++i)Put(p+i,s[i]);}}
namespace rtti {struct Entry{uint32_t rva;const char* name;};const char* Lookup(uint32_t);const char* ClassOf(uintptr_t,uintptr_t);void DetectDelta(uintptr_t,const uintptr_t*,int);bool Ready();}
