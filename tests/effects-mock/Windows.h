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
// Compile-only placeholders for PE discovery; not a Windows ABI test.
struct IMAGE_DOS_HEADER {uint16_t e_magic;int32_t e_lfanew;};
struct IMAGE_FILE_HEADER {uint16_t NumberOfSections,SizeOfOptionalHeader;};
struct IMAGE_NT_HEADERS64 {uint32_t Signature;IMAGE_FILE_HEADER FileHeader;struct {uint16_t Magic;}OptionalHeader;};
struct IMAGE_SECTION_HEADER {struct {uint32_t VirtualSize;}Misc;uint32_t VirtualAddress,Characteristics;};
constexpr uint16_t IMAGE_DOS_SIGNATURE=0x5a4d,IMAGE_NT_OPTIONAL_HDR64_MAGIC=0x20b;
constexpr uint32_t IMAGE_NT_SIGNATURE=0x4550,IMAGE_SCN_MEM_READ=0x40000000,IMAGE_SCN_MEM_WRITE=0x80000000,IMAGE_SCN_MEM_EXECUTE=0x20000000;


#include <vector>
#include <map>
using HWND=void*;using WORD=uint16_t;using LONG=long;using HDC=void*;using HBITMAP=void*;using HGDIOBJ=void*;
struct POINT{LONG x=0,y=0;};struct RECT{LONG left=0,top=0,right=0,bottom=0;};
struct MOUSEINPUT{LONG dx=0,dy=0;DWORD mouseData=0,dwFlags=0,time=0;uintptr_t dwExtraInfo=0;};
struct KEYBDINPUT{WORD wVk=0,wScan=0;DWORD dwFlags=0,time=0;uintptr_t dwExtraInfo=0;};
struct INPUT{DWORD type=0;MOUSEINPUT mi;KEYBDINPUT ki;};
constexpr int INPUT_MOUSE=0,INPUT_KEYBOARD=1,KEYEVENTF_KEYUP=2,MOUSEEVENTF_LEFTDOWN=2,MOUSEEVENTF_LEFTUP=4;
constexpr int VK_F8=18,BI_RGB=0,DIB_RGB_COLORS=0,SRCCOPY=0xcc0020;
struct BITMAPINFOHEADER{DWORD biSize=0;LONG biWidth=0,biHeight=0;WORD biPlanes=0,biBitCount=0;DWORD biCompression=0;};
struct BITMAPINFO{BITMAPINFOHEADER bmiHeader;};
namespace winmock{
 inline uint64_t now=1000;inline HWND foreground=(HWND)1;inline int width=800,height=600;inline POINT cursor{400,300};
 inline std::map<int,int> keys;inline int clicks=0,keyInputs=0;inline std::vector<uint8_t> scene(160*40*4);inline bool captureWorks=true;inline bool cursorWorks=true;inline int nextInputResult=-1;
 struct Dib{std::vector<uint8_t> pixels=std::vector<uint8_t>(160*40*4);};struct Dc{Dib* bitmap=nullptr;};
}
inline int GetAsyncKeyState(int key){int v=winmock::keys[key];winmock::keys[key]=v&~1;return v;}
inline uint64_t GetTickCount64(){return winmock::now;}inline DWORD GetTickCount(){return winmock::now;}inline void Sleep(unsigned){}
inline HWND GetForegroundWindow(){return winmock::foreground;}inline bool IsWindowVisible(HWND h){return h!=nullptr;}
inline DWORD GetWindowThreadProcessId(HWND h,DWORD* p){if(!h)return 0;*p=123;return 1;}inline DWORD GetCurrentProcessId(){return 123;}
inline bool GetClientRect(HWND h,RECT* r){if(!h)return false;r->right=winmock::width;r->bottom=winmock::height;return true;}
inline bool GetCursorPos(POINT* p){*p=winmock::cursor;return true;}inline bool SetCursorPos(int x,int y){if(!winmock::cursorWorks)return false;winmock::cursor={(LONG)x,(LONG)y};return true;}
inline bool ClientToScreen(HWND,POINT*){return true;}inline bool ScreenToClient(HWND,POINT*){return true;}
inline UINT SendInput(UINT n,INPUT* in,int){if(winmock::nextInputResult>=0){n=std::min(n,UINT(winmock::nextInputResult));winmock::nextInputResult=-1;}for(UINT i=0;i<n;++i){if(in[i].type==INPUT_MOUSE&&in[i].mi.dwFlags==MOUSEEVENTF_LEFTDOWN)++winmock::clicks;if(in[i].type==INPUT_KEYBOARD)++winmock::keyInputs;}return n;}
inline HDC GetDC(HWND){return (HDC)1;}inline int ReleaseDC(HWND,HDC){return 1;}
inline HDC CreateCompatibleDC(HDC){return (HDC)new winmock::Dc;}
inline HBITMAP CreateDIBSection(HDC,const BITMAPINFO*,int,void** data,void*,int){auto* bmp=new winmock::Dib;*data=bmp->pixels.data();return bmp;}
inline HGDIOBJ SelectObject(HDC dc,HGDIOBJ bmp){auto* ctx=(winmock::Dc*)dc;auto* old=ctx->bitmap;ctx->bitmap=(winmock::Dib*)bmp;return old;}inline bool DeleteObject(HGDIOBJ bmp){delete (winmock::Dib*)bmp;return true;}inline bool DeleteDC(HDC dc){delete (winmock::Dc*)dc;return true;}
inline bool BitBlt(HDC out,int,int,int,int,HDC,int,int,int){if(!winmock::captureWorks)return false;std::copy(winmock::scene.begin(),winmock::scene.end(),((winmock::Dc*)out)->bitmap->pixels.begin());return true;}
