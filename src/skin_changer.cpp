#include "skin_changer.h"
#include "auto_accept.h"
#include <Windows.h>
#include <mutex>
#include <atomic>
#include <fstream>
#include <sstream>
#include <chrono>
#include "cosmetic_catalog.inc"
namespace skins {
using namespace cosmetic;
static std::mutex guard;
static Core core(cosmeticDefinitions,sizeof(cosmeticDefinitions)/sizeof(*cosmeticDefinitions));
using SendFn=int(__fastcall*)(void*,uint32_t,const void*,uint32_t);
using AvailFn=bool(__fastcall*)(void*,uint32_t*);
using RetrieveFn=int(__fastcall*)(void*,uint32_t*,void*,uint32_t,uint32_t*);
static SendFn originalSend=nullptr;static AvailFn originalAvail=nullptr;static RetrieveFn originalRetrieve=nullptr;
static void* gc=nullptr;static void** oldVtable=nullptr;static void* newVtable[3]={};
static bool connected=false;static std::atomic<unsigned> activeCalls{0};
static std::string message="Not connected; experimental local inventory projection.";
static Packet staged,stagedRaw;static bool haveStaged=false;
static std::map<Slot,std::pair<uint32_t,uint32_t>> saved;
static bool needRestore=false;
struct Call {Call(){++activeCalls;}~Call(){--activeCalls;}};
static std::wstring ProfilePath(){wchar_t path[32768]={};HMODULE mod=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&ProfilePath),&mod))return {};DWORD n=GetModuleFileNameW(mod,path,32768);if(!n||n>=32768)return {};std::wstring file(path,n);size_t pos=file.find_last_of(L"\\/");if(pos==std::wstring::npos)return {};file.resize(pos+1);return file+L"cosmetic-local.ini";}
static bool Save(){auto path=ProfilePath();if(path.empty())return false;std::ostringstream text;for(const auto& c:core.SelectedDefinitions())text<<c.first.first<<','<<c.first.second<<','<<c.second.first<<','<<c.second.second<<';';std::string s=text.str();if(s.size()>32000)return false;std::wstring w(s.begin(),s.end());return WritePrivateProfileStringW(L"local",L"choices",w.c_str(),path.c_str())!=0;}
static void Load(){auto path=ProfilePath();if(path.empty())return;wchar_t data[32768]={};GetPrivateProfileStringW(L"local",L"choices",L"",data,32768,path.c_str());std::wstring w(data);std::string s(w.begin(),w.end());std::istringstream stream(s);std::string item;while(std::getline(stream,item,';')){std::replace(item.begin(),item.end(),',',' ');std::istringstream row(item);uint32_t cls,slot,def,style;if(row>>cls>>slot>>def>>style)saved[{cls,slot}]={def,style};}needRestore=!saved.empty();}
static void ApplySaved(){if(!needRestore||!core.Ready()||!core.Enabled())return;needRestore=false;core.RestoreSelections(saved);}
static bool Executable(void* fn){MEMORY_BASIC_INFORMATION info{};if(!fn||VirtualQuery(fn,&info,sizeof(info))!=sizeof(info)||info.State!=MEM_COMMIT||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;return (info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;}
static bool Stage(){// Must be called under guard. Never consume a message and silently lose it on a small caller buffer.
 if(haveStaged)return true;uint32_t size=0;if(!originalAvail(gc,&size))return false;if(size>pb::MaxBytes){message="GC message exceeds local limit; forwarded without processing.";return false;}Bytes raw(std::max(uint32_t(8),size));uint32_t type=0,actual=0;int result=originalRetrieve(gc,&type,raw.data(),uint32_t(raw.size()),&actual);if(result==2&&actual<=pb::MaxBytes){raw.resize(actual);result=originalRetrieve(gc,&type,raw.data(),actual,&actual);}if(result!=0)return false;if(actual>raw.size()){message="Invalid GC length.";return false;}raw.resize(actual);bool wasReady=core.Ready();stagedRaw={type,std::move(raw)};autoaccept::Observe(type,stagedRaw.data);staged=core.Incoming(type,stagedRaw.data);if(!wasReady&&core.Ready())message="Genuine local cache captured. In-match cosmetic models remain unverified.";haveStaged=true;try{ApplySaved();}catch(const std::exception& e){message=e.what();}return true;}
static int __fastcall Send(void* self,uint32_t type,const void* data,uint32_t size){Call call;if(self!=gc)return originalSend(self,type,data,size);if(size>pb::MaxBytes||(!data&&size))return 4;int decision=-1;try{Bytes b;if(size)b.assign(static_cast<const uint8_t*>(data),static_cast<const uint8_t*>(data)+size);std::lock_guard<std::mutex> lock(guard);decision=core.Outgoing(type,b);if(decision==0){Save();message="Equip/style handled locally; not sent to Steam.";}else if(decision==4)message="Local operation rejected: invalid item, slot, style or cache.";}catch(...){decision=4;}return decision<0?originalSend(self,type,data,size):decision;}
static bool __fastcall Available(void* self,uint32_t* size){Call call;if(self!=gc||!size)return originalAvail(self,size);try{std::lock_guard<std::mutex> lock(guard);if(haveStaged){*size=uint32_t(staged.data.size());return true;}if(core.HasQueued()){*size=core.QueuedSize();return true;}if(Stage()){*size=uint32_t(staged.data.size());return true;}}catch(const std::exception& e){message=e.what();}return originalAvail(self,size);}
static int __fastcall Retrieve(void* self,uint32_t* type,void* destination,uint32_t cap,uint32_t* size){Call call;if(self!=gc||!type||!size||(!destination&&cap))return originalRetrieve(self,type,destination,cap,size);try{std::lock_guard<std::mutex> lock(guard);if(haveStaged){*size=uint32_t(staged.data.size());*type=staged.type;if(cap<*size)return 2;if(!staged.data.empty())memcpy(destination,staged.data.data(),staged.data.size());haveStaged=false;staged={};stagedRaw={};return 0;}if(core.HasQueued()){*size=core.QueuedSize();if(cap<*size)return 2;Packet p;core.Pop(p);*type=p.type;if(!p.data.empty())memcpy(destination,p.data.data(),p.data.size());return 0;}if(Stage()){*size=uint32_t(staged.data.size());*type=staged.type;if(cap<*size)return 2;if(!staged.data.empty())memcpy(destination,staged.data.data(),staged.data.size());haveStaged=false;staged={};stagedRaw={};return 0;}}catch(const std::exception& e){message=e.what();return 4;}return originalRetrieve(self,type,destination,cap,size);}
static void Connect(){HMODULE api=GetModuleHandleW(L"steam_api64.dll");if(!api){message="steam_api64.dll is not loaded.";return;}
 using Create=void*(__cdecl*)(const char*);using Handle=int(__cdecl*)();using Generic=void*(__cdecl*)(void*,int,int,const char*);using SteamId=uint64_t(__cdecl*)(void*);
 auto create=reinterpret_cast<Create>(GetProcAddress(api,"SteamInternal_CreateInterface"));auto userHandle=reinterpret_cast<Handle>(GetProcAddress(api,"SteamAPI_GetHSteamUser"));auto pipeHandle=reinterpret_cast<Handle>(GetProcAddress(api,"SteamAPI_GetHSteamPipe"));auto generic=reinterpret_cast<Generic>(GetProcAddress(api,"SteamAPI_ISteamClient_GetISteamGenericInterface"));auto userGet=reinterpret_cast<Generic>(GetProcAddress(api,"SteamAPI_ISteamClient_GetISteamUser"));auto steamId=reinterpret_cast<SteamId>(GetProcAddress(api,"SteamAPI_ISteamUser_GetSteamID"));if(!create||!userHandle||!pipeHandle||!generic||!userGet||!steamId){message="Required Steam flat API exports are missing; no offsets guessed.";return;}
 void* client=nullptr;for(const char* v:{"SteamClient023","SteamClient022","SteamClient021","SteamClient020"})if((client=create(v)))break;if(!client){message="Supported SteamClient interface unavailable.";return;}
 int h=userHandle(),p=pipeHandle();if(!h||!p){message="Steam handles are not ready.";return;}void* user=nullptr;for(const char* v:{"SteamUser023","SteamUser022","SteamUser021","SteamUser020"})if((user=userGet(client,h,p,v)))break;if(!user){message="SteamUser unavailable.";return;}uint64_t owner=steamId(user);if(!owner){message="Steam account not ready.";return;}
 void* coordinator=generic(client,h,p,"SteamGameCoordinator001");if(!coordinator){message="SteamGameCoordinator001 unavailable.";return;}void** vt=*static_cast<void***>(coordinator);if(!vt||!Executable(vt[0])||!Executable(vt[1])||!Executable(vt[2])){message="GC interface validation failed; hooks not installed.";return;}
 gc=coordinator;oldVtable=vt;originalSend=reinterpret_cast<SendFn>(vt[0]);originalAvail=reinterpret_cast<AvailFn>(vt[1]);originalRetrieve=reinterpret_cast<RetrieveFn>(vt[2]);newVtable[0]=reinterpret_cast<void*>(&Send);newVtable[1]=reinterpret_cast<void*>(&Available);newVtable[2]=reinterpret_cast<void*>(&Retrieve);
 // Replace only this three-method interface's vtable pointer, not executable instructions.
 auto previous=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(coordinator),newVtable,vt);if(previous!=vt){message="GC interface changed; installation aborted.";return;}
 core.SetOwner(owner);connected=true;Load();message="GC connected. Enable, then refresh the local inventory.";}
const Definition* Catalog(size_t& count){count=sizeof(cosmeticDefinitions)/sizeof(*cosmeticDefinitions);return cosmeticDefinitions;}
void Poll(){static auto next=std::chrono::steady_clock::time_point{};auto now=std::chrono::steady_clock::now();if(now<next)return;next=now+std::chrono::seconds(2);std::lock_guard<std::mutex> lock(guard);if(!connected)try{Connect();}catch(...){message="Steam interface setup failed; module inactive.";}}
Status GetStatus(){std::lock_guard<std::mutex> lock(guard);Status s;s.connected=connected;s.enabled=core.Enabled();s.cacheReady=core.Ready();s.definitions=core.Count();s.selections=core.Selected();s.account=core.OwnerId();s.pending=core.HasQueued()?1:0;s.message=message;return s;}
bool SetEnabled(bool on){
 {std::lock_guard<std::mutex> lock(guard);
  if(on&&!connected){message="Cannot enable: GC interface not connected.";return false;}
  if(core.Enabled()==on)return true;
  try{core.Enable(on);if(!on&&haveStaged)staged=core.Incoming(stagedRaw.type,stagedRaw.data);if(on)ApplySaved();
   message=on?"Native armory projection enabled; requesting the original account cache automatically.":"Original armory restore queued; wait before unloading.";
  }catch(const std::exception& e){message=e.what();return false;}
 }
 // Trigger a genuine GC response/callback; never require an extra first-use button.
 if(on)Refresh();return true;
}
bool Refresh(){SendFn fn;void* self;Packet p;{std::lock_guard<std::mutex> lock(guard);if(!connected){message="GC not connected.";return false;}fn=originalSend;self=gc;p=core.Refresh();}int r=fn(self,p.type,p.data.data(),uint32_t(p.data.size()));std::lock_guard<std::mutex> lock(guard);message=r==0?"Cache refresh requested. Waiting for a genuine GC response.":"GC rejected the refresh request.";return r==0;}
bool Select(uint32_t def,uint32_t cls,uint32_t slot,uint32_t style){std::lock_guard<std::mutex> lock(guard);try{if(!core.Select(def,cls,slot,style)){message="Cannot equip: enable and refresh first, then select a valid slot/style.";return false;}bool savedOk=Save();message=savedOk?"Local equip queued; not sent to Steam. Profile saved.":"Local equip queued; could not save the profile.";return true;}catch(const std::exception& e){message=e.what();return false;}}
bool Reset(){std::lock_guard<std::mutex> lock(guard);try{core.Reset();saved.clear();needRestore=false;bool ok=Save();message=ok?"Local selections cleared; original equipped states restored locally.":"Local selections cleared; profile save failed.";return true;}catch(const std::exception& e){message=e.what();return false;}}
void NotifyUnloadBlocked(){std::lock_guard<std::mutex> lock(guard);message="Before unloading, disable local cosmetics and wait for queued cache restoration. If the game does not poll the queue, close Dota instead.";}
bool CanUnload(){std::lock_guard<std::mutex> lock(guard);return !core.Enabled()&&!core.HasQueued()&&activeCalls.load()==0;}
void Shutdown(){std::lock_guard<std::mutex> lock(guard);if(core.Enabled()||core.HasQueued())return;if(connected&&gc)InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(gc),oldVtable,newVtable);connected=false;}
}
