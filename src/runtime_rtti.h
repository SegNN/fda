#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace runtimertti {
struct Locator {uint32_t signature,offset,cdOffset,typeRva,hierarchyRva,selfRva;};
template<class Reader>
bool Name(Reader& r,uintptr_t base,uint32_t imageSize,uintptr_t vptr,char* out,size_t capacity){
 if(!out||capacity<2)return false;out[0]=0;
 if(!base||imageSize<4096||imageSize>0x40000000u||base+imageSize<base)return false;
 auto inside=[&](uintptr_t p,size_t n){return p>=base&&p<=base+imageSize&&n<=base+imageSize-p;};
 if((vptr&7)||!inside(vptr-8,16))return false;
 uintptr_t locator=0;Locator c{};
 if(!r.Read(vptr-8,locator)||!inside(locator,sizeof(c))||!r.Read(locator,c)||c.signature!=1||c.selfRva!=locator-base||c.offset!=0||c.cdOffset!=0)return false;
 if(!c.typeRva||!c.hierarchyRva||!inside(base+c.typeRva,17)||!inside(base+c.hierarchyRva,16))return false;
 char decorated[128]={};bool terminated=false;
 for(size_t i=0;i<sizeof(decorated)-1;++i){if(!inside(base+c.typeRva+16+i,1)||!r.Read(base+c.typeRva+16+i,decorated[i]))return false;if(!decorated[i]){terminated=true;break;}}
 if(!terminated||strncmp(decorated,".?A",3)||(decorated[3]!='V'&&decorated[3]!='U'))return false;
 const char* begin=decorated+4;size_t len=strlen(begin);
 if(len<3||strcmp(begin+len-2,"@@"))return false;len-=2;if(len>=capacity)return false;
 for(size_t i=0;i<len;++i)if(!((begin[i]>='a'&&begin[i]<='z')||(begin[i]>='A'&&begin[i]<='Z')||(begin[i]>='0'&&begin[i]<='9')||begin[i]=='_'))return false;
 memcpy(out,begin,len);out[len]=0;return true;
}
}
