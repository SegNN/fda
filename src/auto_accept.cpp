#include "auto_accept.h"
#include "auto_accept_core.h"
#include "common.h"
#include <mutex>
#include <fstream>
#include <filesystem>
namespace autoaccept {
static std::mutex guard;static acceptcore::Gate gate;static acceptcore::Image sample;
static int screenW=0,screenH=0,centerX=0,centerY=0;static bool loaded=false;
static std::string status="No button template. During a ready-up, hover Accept and press F8.";
static std::filesystem::path Path(){wchar_t file[32768]={};HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&Path),&module))return {};
 DWORD n=GetModuleFileNameW(module,file,32768);if(!n||n>=32768)return {};return std::filesystem::path(file).parent_path()/L"accept-button.bin";}
static void Load(){if(loaded)return;loaded=true;auto path=Path();if(path.empty())return;std::ifstream f(path,std::ios::binary);uint32_t magic=0;f.read((char*)&magic,4);f.read((char*)&screenW,4);f.read((char*)&screenH,4);f.read((char*)&centerX,4);f.read((char*)&centerY,4);
 if(!f||magic!=0x33414346||screenW<160||screenH<40||screenW>16000||screenH>16000||centerX<80||centerY<20||centerX>screenW-80||centerY>screenH-20)return;
 acceptcore::Image im;im.width=160;im.height=40;im.rgb.resize(19200);f.read((char*)im.rgb.data(),19200);char trailing=0;
 if(f && !f.read(&trailing,1) && acceptcore::Informative(im)){sample=std::move(im);status="Button template loaded; waiting for a fresh GC ready-up.";}}
static bool Save(){auto path=Path();if(path.empty())return false;std::ofstream f(path,std::ios::binary|std::ios::trunc);uint32_t magic=0x33414346;f.write((char*)&magic,4);f.write((char*)&screenW,4);f.write((char*)&screenH,4);f.write((char*)&centerX,4);f.write((char*)&centerY,4);f.write((char*)sample.rgb.data(),sample.rgb.size());return bool(f);}
static bool OwnWindow(HWND hwnd){DWORD pid=0;return hwnd&&IsWindowVisible(hwnd)&&GetWindowThreadProcessId(hwnd,&pid)&&pid==GetCurrentProcessId();}
static bool Capture(HWND hwnd,int x,int y,acceptcore::Image& out){
 HDC dc=GetDC(hwnd);if(!dc)return false;HDC copy=CreateCompatibleDC(dc);if(!copy){ReleaseDC(hwnd,dc);return false;}
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=160;info.bmiHeader.biHeight=-40;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* data=nullptr;HBITMAP bmp=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&data,nullptr,0);bool ok=false;
 if(bmp&&data){HGDIOBJ old=SelectObject(copy,bmp);if(BitBlt(copy,0,0,160,40,dc,x-80,y-20,SRCCOPY)){
   out.width=160;out.height=40;out.rgb.resize(19200);auto bytes=(const uint8_t*)data;
   for(int i=0;i<6400;++i){out.rgb[i*3]=bytes[i*4+2];out.rgb[i*3+1]=bytes[i*4+1];out.rgb[i*3+2]=bytes[i*4];}ok=true;
  }SelectObject(copy,old);DeleteObject(bmp);}
 DeleteDC(copy);ReleaseDC(hwnd,dc);return ok;
}
void Observe(uint32_t type,const cosmetic::Bytes& raw){std::lock_guard<std::mutex> lock(guard);gate.Observe(type,raw,GetTickCount64());}
void Tick(bool inMatch){
 std::lock_guard<std::mutex> lock(guard);Load();uint64_t now=GetTickCount64();gate.Enable(cfg::autoAccept,now);
 if(!cfg::autoAccept)return;
 bool calibrate=binds::Edge[VK_F8]; // ProcessBinds already consumed GetAsyncKeyState low bits.
 HWND hwnd=GetForegroundWindow();bool foreground=OwnWindow(hwnd);
 if(inMatch||cfg::menuOpen||!foreground){gate.Sample(now,false,foreground,!cfg::menuOpen,!inMatch,!sample.rgb.empty());return;}
 if(!gate.Fresh(now)){status=gate.Confirmed()?"GC reports local ready state accepted.":"Waiting for fresh GC ReadyUpStatus (undeclared state or schema default).";return;}
 RECT bounds{};if(!GetClientRect(hwnd,&bounds))return;int w=bounds.right,h=bounds.bottom;
 if(calibrate){POINT pos{};if(!GetCursorPos(&pos)||!ScreenToClient(hwnd,&pos)||pos.x<80||pos.y<20||pos.x>w-80||pos.y>h-20)return;
  acceptcore::Image image;if(!Capture(hwnd,pos.x,pos.y,image)||!acceptcore::Informative(image)){status="Calibration failed: empty/unreadable button capture.";return;}
  sample=std::move(image);centerX=pos.x;centerY=pos.y;screenW=w;screenH=h;
  status=Save()?"Button sample saved. This invitation must be accepted manually; next one can be automatic.":"Sample works this session; saving failed. Accept this invitation manually.";
  gate.Attempt();return;
 }
 if(sample.rgb.empty()){status="Hover the native Accept button during this invite, then press F8; accept manually once.";return;}
 if(w!=screenW||h!=screenH){status="Client size changed. Recalibrate the button with F8.";return;}
 static uint64_t nextCapture=0;if(now<nextCapture)return;nextCapture=now+250;
 POINT hover{centerX,centerY};if(!ClientToScreen(hwnd,&hover)||GetForegroundWindow()!=hwnd||!SetCursorPos(hover.x,hover.y))return;
 acceptcore::Image image;bool match=Capture(hwnd,centerX,centerY,image)&&acceptcore::Matches(sample,image);
 if(!gate.Sample(now,match,foreground,true,true,true)){status=match?"Accept button verified; waiting for stable frames.":"Ready-up received, but the button image does not match. No click.";return;}
 // Final visual/focus revalidation immediately before one native click.
 if(GetForegroundWindow()!=hwnd||cfg::menuOpen||!cfg::autoAccept||!gate.Fresh(GetTickCount64())||!Capture(hwnd,centerX,centerY,image)||!acceptcore::Matches(sample,image))return;
 POINT target{centerX,centerY};if(!ClientToScreen(hwnd,&target))return;
 gate.Attempt();if(!SetCursorPos(target.x,target.y)){status="Cursor move failed; invitation not clicked.";return;}
 if(GetForegroundWindow()!=hwnd||cfg::menuOpen||!cfg::autoAccept){status="Focus/settings changed before click; aborted.";return;}
 INPUT click[2]{};click[0].type=click[1].type=INPUT_MOUSE;click[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;click[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
 UINT sent=SendInput(2,click,sizeof(INPUT));if(sent==1)SendInput(1,&click[1],sizeof(INPUT));
 status=sent==2?"Native Accept click sent once. Acceptance still needs confirmation in Dota.":"Click delivery failed; not retried for this invitation.";
}
std::string Status(){std::lock_guard<std::mutex> lock(guard);Load();return status;}
void ClearTemplate(){std::lock_guard<std::mutex> lock(guard);sample={};std::error_code ec;auto path=Path();if(!path.empty())std::filesystem::remove(path,ec);status=ec?"Sample disabled this session, but deleting its file failed. Remove accept-button.bin manually.":"Button sample removed. Calibration required.";}
}
