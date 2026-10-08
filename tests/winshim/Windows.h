#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <algorithm>
using HMODULE=void*;using LPCWSTR=const wchar_t*;using DWORD=unsigned long;using UINT=unsigned;using ULONGLONG=unsigned long long;
constexpr int MAX_PATH=260,CP_UTF8=65001,GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS=4,GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT=2;
constexpr DWORD INVALID_FILE_ATTRIBUTES=0xFFFFFFFF;
inline bool GetModuleHandleExW(int,LPCWSTR,HMODULE*){return false;}
inline DWORD GetModuleFileNameW(HMODULE,wchar_t*,DWORD){return 0;}
inline DWORD GetFileAttributesW(const wchar_t*){return INVALID_FILE_ATTRIBUTES;}
inline bool WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return false;}
inline UINT GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return d;}
inline DWORD GetPrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t* def,wchar_t* dest,int cap,const wchar_t*){wcsncpy(dest,def,cap);return wcslen(def);}
inline int MultiByteToWideChar(int,int,const char*,int,wchar_t*,int){return 0;}
inline int WideCharToMultiByte(int,int,const wchar_t*,int,char*,int,void*,void*){return 0;}
template<size_t N,typename... T>int swprintf_s(wchar_t(&b)[N],const wchar_t* fmt,T... t){return swprintf(b,N,fmt,t...);}
template<size_t N>void wcscat_s(wchar_t(&b)[N],const wchar_t* s){wcsncat(b,s,N-wcslen(b)-1);}
template<size_t N>void strcpy_s(char(&b)[N],const char* s){strncpy(b,s,N);}
#define VK_ADD 1
#define VK_BACK 2
#define VK_CAPITAL 3
#define VK_CONTROL 4
#define VK_DECIMAL 5
#define VK_DELETE 6
#define VK_DIVIDE 7
#define VK_DOWN 8
#define VK_END 9
#define VK_ESCAPE 10
#define VK_F1 11
#define VK_F24 12
#define VK_HOME 13
#define VK_INSERT 14
#define VK_LBUTTON 15
#define VK_LCONTROL 16
#define VK_LEFT 17
#define VK_LMENU 18
#define VK_LSHIFT 19
#define VK_MBUTTON 20
#define VK_MENU 21
#define VK_MULTIPLY 22
#define VK_NEXT 23
#define VK_NUMPAD0 24
#define VK_NUMPAD9 25
#define VK_OEM_1 26
#define VK_OEM_2 27
#define VK_OEM_3 28
#define VK_OEM_4 29
#define VK_OEM_5 30
#define VK_OEM_6 31
#define VK_OEM_7 32
#define VK_OEM_COMMA 33
#define VK_OEM_MINUS 34
#define VK_OEM_PERIOD 35
#define VK_OEM_PLUS 36
#define VK_PAUSE 37
#define VK_PRIOR 38
#define VK_RBUTTON 39
#define VK_RCONTROL 40
#define VK_RETURN 41
#define VK_RIGHT 42
#define VK_RMENU 43
#define VK_RSHIFT 44
#define VK_SCROLL 45
#define VK_SHIFT 46
#define VK_SNAPSHOT 47
#define VK_SPACE 48
#define VK_SUBTRACT 49
#define VK_TAB 50
#define VK_UP 51
#define VK_XBUTTON1 52
#define VK_XBUTTON2 53

constexpr int FALSE=0;
#ifndef __fastcall
#define __fastcall
#define __cdecl
#endif
struct MEMORY_BASIC_INFORMATION {DWORD State=0,Protect=0;};
constexpr DWORD MEM_COMMIT=0x1000,PAGE_GUARD=0x100,PAGE_NOACCESS=1,PAGE_EXECUTE=0x10,PAGE_EXECUTE_READ=0x20,PAGE_EXECUTE_READWRITE=0x40,PAGE_EXECUTE_WRITECOPY=0x80;
inline size_t VirtualQuery(const void*,MEMORY_BASIC_INFORMATION* p,size_t n){p->State=MEM_COMMIT;p->Protect=PAGE_EXECUTE_READ;return n;}
inline void* InterlockedCompareExchangePointer(void* volatile* p,void* value,void* compare){void* old=*p;if(old==compare)*p=value;return old;}
HMODULE GetModuleHandleW(const wchar_t*);
void* GetProcAddress(HMODULE,const char*);
