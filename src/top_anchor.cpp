#include "top_anchor.h"
#include "imgui.h"
#include <wincodec.h>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <utility>

namespace topanchor {
Stats stats;
static ID3D11Device* g_device=nullptr;
static ID3D11Texture2D* g_staging=nullptr;
static IWICImagingFactory* g_factory=nullptr;
static std::wstring g_assets;
static std::vector<float> g_gray;
static int g_width=0,g_height=0,g_bufferHeight=0;
static DXGI_FORMAT g_format=DXGI_FORMAT_UNKNOWN;
static ULONGLONG g_nextCapture=0;
struct CachedAnchor { Anchor value; ULONGLONG verifiedAt=0; };
static std::unordered_map<std::string,CachedAnchor> g_anchors;
static std::string g_roster,g_pendingRoster;
static ULONGLONG g_rosterSince=0;
static ULONGLONG g_invalidSince=0;
struct ScaledTemplate {int w=0,h=0;std::vector<float> centered;float energy=0;};
struct Job {
    std::string key;
    int team=0;
    std::array<float,84> normalized{};
    float templateEnergy=0;
    int xLo=0,xHi=0,wLo=0,wCount=0,xCount=0,yCount=13;
    size_t cursor=0,total=0;
    Anchor best;
    std::vector<float> portrait;
    int portraitWidth=0,portraitHeight=0;
    bool refining=false;
    int seedX=0,seedY=0;
    size_t refineCursor=0;
    std::vector<ScaledTemplate> scales;
};
static std::vector<Job> g_jobs;
static size_t g_job=0;
static const char* HeroKey(const FrameUnit& u) {
    return StartsWith(u.name,"npc_dota_hero_")?u.name+strlen("npc_dota_hero_"):u.nick;
}
static std::string Key(const FrameUnit& u){return std::to_string(u.team)+"/"+HeroKey(u);}
static std::wstring AssetRoot() {
    wchar_t path[MAX_PATH]={};HMODULE module=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&AssetRoot),&module))return {};
    DWORD n=GetModuleFileNameW(module,path,MAX_PATH);if(!n||n>=MAX_PATH)return {};
    std::wstring dir(path);size_t at=dir.find_last_of(L"\\/");if(at==std::wstring::npos)return {};
    dir.resize(at+1);std::wstring result=dir+L"assets\\portraits\\";
    if(GetFileAttributesW(result.c_str())==INVALID_FILE_ATTRIBUTES)result=dir+L"..\\assets\\portraits\\";
    return result;
}
void Initialize(ID3D11Device* device){Shutdown();if(device){g_device=device;g_device->AddRef();}g_assets=AssetRoot();
    // hud::Initialize has initialized COM on this rendering thread.
    CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g_factory));}
void Shutdown(){if(g_staging){g_staging->Release();g_staging=nullptr;}if(g_factory){g_factory->Release();g_factory=nullptr;}if(g_device){g_device->Release();g_device=nullptr;}
    g_jobs.clear();g_anchors.clear();g_roster.clear();g_invalidSince=0;g_gray.clear();g_job=0;g_width=0;g_height=0;g_format=DXGI_FORMAT_UNKNOWN;g_nextCapture=0;stats=Stats{};}
static bool Template(const char* name,Job& job) {
    if(!g_factory||!name||!*name)return false;
    for(const char* c=name;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||*c=='_'))return false;
    std::wstring file=g_assets+std::wstring(name,name+strlen(name))+L".png";
    IWICBitmapDecoder* decoder=nullptr;IWICBitmapFrameDecode* frame=nullptr;IWICFormatConverter* converter=nullptr;
    UINT w=0,h=0;HRESULT hr=g_factory->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder);
    if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);if(SUCCEEDED(hr))hr=frame->GetSize(&w,&h);
    if(SUCCEEDED(hr)&&(!w||!h||w>1024||h>1024))hr=E_INVALIDARG;
    if(SUCCEEDED(hr))hr=g_factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr))hr=converter->Initialize(frame,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    bool ok=false;
    if(SUCCEEDED(hr)){
        std::vector<unsigned char> pixels((size_t)w*h*4);hr=converter->CopyPixels(nullptr,w*4,(UINT)pixels.size(),pixels.data());
        if(SUCCEEDED(hr)){
            job.portraitWidth=(int)w;job.portraitHeight=(int)h;job.portrait.resize((size_t)w*h);
            for(size_t i=0;i<job.portrait.size();++i){const auto* p=&pixels[i*4];job.portrait[i]=.299f*p[0]+.587f*p[1]+.114f*p[2];}
            float mean=0;int index=0;
            for(int y=0;y<7;++y)for(int x=0;x<12;++x){UINT sx=(UINT)((x+.5f)*w/12),sy=(UINT)((y+.5f)*h/7);const auto* p=&pixels[((size_t)sy*w+sx)*4];float gray=.299f*p[0]+.587f*p[1]+.114f*p[2];job.normalized[index++]=gray;mean+=gray;}
            mean/=84;for(float& v:job.normalized){v-=mean;job.templateEnergy+=v*v;}ok=job.templateEnergy>100.f;
        }
    }
    if(converter)converter->Release();if(frame)frame->Release();if(decoder)decoder->Release();return ok;
}
static bool Capture(IDXGISwapChain* chain) {
    if(!chain||!g_device)return false;
    ID3D11Texture2D* buffer=nullptr;if(FAILED(chain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&buffer)))return false;
    D3D11_TEXTURE2D_DESC desc{};buffer->GetDesc(&desc);
    bool rgba=desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    bool bgra=desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    if((!rgba&&!bgra)||desc.SampleDesc.Count!=1||desc.Width>7680||desc.Height<32){buffer->Release();return false;}
    int roiHeight=std::min(160,(int)desc.Height);
    if(g_width!=(int)desc.Width||g_height!=roiHeight||g_bufferHeight!=(int)desc.Height||g_format!=desc.Format){if(g_staging){g_staging->Release();g_staging=nullptr;}g_anchors.clear();g_width=(int)desc.Width;g_height=roiHeight;g_format=desc.Format;}
    g_bufferHeight=(int)desc.Height;
    if(!g_staging){D3D11_TEXTURE2D_DESC staging=desc;staging.Height=roiHeight;staging.MipLevels=1;staging.ArraySize=1;staging.Usage=D3D11_USAGE_STAGING;staging.BindFlags=0;staging.CPUAccessFlags=D3D11_CPU_ACCESS_READ;staging.MiscFlags=0;
        if(FAILED(g_device->CreateTexture2D(&staging,nullptr,&g_staging))){buffer->Release();return false;}}
    ID3D11DeviceContext* ctx=nullptr;g_device->GetImmediateContext(&ctx);
    D3D11_BOX box{0,0,0,desc.Width,(UINT)roiHeight,1};ctx->CopySubresourceRegion(g_staging,0,0,0,0,buffer,0,&box);
    D3D11_MAPPED_SUBRESOURCE mapped{};HRESULT hr=ctx->Map(g_staging,0,D3D11_MAP_READ,0,&mapped);
    if(SUCCEEDED(hr)){g_gray.resize((size_t)g_width*g_height);
        for(int y=0;y<g_height;++y){const auto* row=(const unsigned char*)mapped.pData+(size_t)y*mapped.RowPitch;
            for(int x=0;x<g_width;++x){const auto* p=row+x*4;g_gray[(size_t)y*g_width+x]=.299f*p[rgba?0:2]+.587f*p[1]+.114f*p[rgba?2:0];}}
        ctx->Unmap(g_staging,0);}
    ctx->Release();buffer->Release();return SUCCEEDED(hr);
}
static float Score(const Job& job,int x,int y,int w,int h) {
    if(x<0||y<0||x+w>g_width||y+h>g_height)return -1;
    float sum=0,squares=0,covariance=0;int index=0;
    for(int yy=0;yy<7;++yy)for(int xx=0;xx<12;++xx){int sx=x+(int)((xx+.5f)*w/12),sy=y+(int)((yy+.5f)*h/7);float v=g_gray[(size_t)sy*g_width+sx];sum+=v;squares+=v*v;covariance+=v*job.normalized[index++];}
    float energy=squares-sum*sum/84.f;if(energy<100.f)return -1;
    return covariance/sqrtf(energy*job.templateEnergy);
}
static float Interpolate(const Job& job,float x,float y) {
    x=std::max(0.f,std::min((float)job.portraitWidth-1,x));y=std::max(0.f,std::min((float)job.portraitHeight-1,y));
    int x0=(int)x,y0=(int)y,x1=std::min(x0+1,job.portraitWidth-1),y1=std::min(y0+1,job.portraitHeight-1);float ax=x-x0,ay=y-y0;
    float a=job.portrait[(size_t)y0*job.portraitWidth+x0]*(1-ax)+job.portrait[(size_t)y0*job.portraitWidth+x1]*ax;
    float b=job.portrait[(size_t)y1*job.portraitWidth+x0]*(1-ax)+job.portrait[(size_t)y1*job.portraitWidth+x1]*ax;return a*(1-ay)+b*ay;
}
static void PrepareRefinement(Job& job) {
    job.seedX=(int)job.best.x;job.seedY=(int)job.best.y;
    int baseWidth=(int)job.best.w;
    for(int w=std::max(24,baseWidth-4);w<=baseWidth+4;++w){ScaledTemplate t;t.w=w;t.h=(int)(w*9.f/16.f+.5f);t.centered.resize((size_t)t.w*t.h);float mean=0;
        for(int y=0;y<t.h;++y)for(int x=0;x<t.w;++x){float v=Interpolate(job,(x+.5f)*job.portraitWidth/t.w-.5f,(y+.5f)*job.portraitHeight/t.h-.5f);t.centered[(size_t)y*t.w+x]=v;mean+=v;}
        mean/=(float)t.centered.size();for(float& v:t.centered){v-=mean;t.energy+=v*v;}job.scales.push_back(std::move(t));}
    job.best=Anchor{};job.refining=true;
}
static float FullScore(const ScaledTemplate& t,int x,int y) {
    if(x<0||y<0||x+t.w>g_width||y+t.h>g_height)return -1;
    float sum=0,squares=0,cov=0;
    for(int yy=0;yy<t.h;++yy){const float* row=&g_gray[(size_t)(y+yy)*g_width+x];const float* tmp=&t.centered[(size_t)yy*t.w];
        for(int xx=0;xx<t.w;++xx){float v=row[xx];sum+=v;squares+=v*v;cov+=v*tmp[xx];}}
    float energy=squares-sum*sum/(float)t.centered.size();return energy>100.f&&t.energy>100.f?cov/sqrtf(energy*t.energy):-1.f;
}

void Update(IDXGISwapChain* chain,const Frame& frame) {
    ULONGLONG now=GetTickCount64();
    if(!cfg::hudTop){g_anchors.clear();g_jobs.clear();g_roster.clear();stats=Stats{};return;}
    if(!frame.ok){if(!g_invalidSince)g_invalidSince=now;if(now-g_invalidSince>2000){g_anchors.clear();g_jobs.clear();g_roster.clear();}return;}
    g_invalidSince=0;
    std::vector<std::string> roster;
    for(const auto& u:frame.units)if(u.kind==UnitKind::Hero&&!u.illusion&&(u.team==2||u.team==3))roster.push_back(Key(u)+"/"+std::to_string(u.playerId));
    std::sort(roster.begin(),roster.end());std::string signature;for(const auto& key:roster)signature+=key+";";
    if(signature!=g_roster){
        if(signature!=g_pendingRoster){g_pendingRoster=signature;g_rosterSince=now;}
        if(g_roster.empty()||now-g_rosterSince>=2000){g_roster=signature;g_anchors.clear();g_jobs.clear();g_job=0;g_nextCapture=0;}
    }else{g_pendingRoster.clear();g_rosterSince=0;}
    for(auto it=g_anchors.begin();it!=g_anchors.end();)if(now-it->second.verifiedAt>30000)it=g_anchors.erase(it);else ++it;
    if(g_job>=g_jobs.size()&&now>=g_nextCapture){
        g_jobs.clear();g_job=0;stats=Stats{};stats.captureOk=Capture(chain);g_nextCapture=now+5000;
        if(!stats.captureOk)return; // short capture failure does not destroy confirmed geometry
        std::unordered_map<std::string,int> counts;for(const auto& u:frame.units)if(u.kind==UnitKind::Hero&&!u.illusion&&(u.team==2||u.team==3))++counts[Key(u)];
        for(const auto& u:frame.units){if(u.kind!=UnitKind::Hero||u.illusion||(u.team!=2&&u.team!=3))continue;std::string key=Key(u);
            if(counts[key]!=1){g_anchors.erase(key);continue;}Job job;job.key=key;job.team=u.team;if(!Template(HeroKey(u),job)){g_anchors.erase(key);continue;}
            // Broad ROI bounds are only search limits; actual positions come from pixels.
            job.xLo=u.team==2?(int)(g_width*.15f):g_width/2+20;job.xHi=u.team==2?g_width/2-20:(int)(g_width*.85f);
            float estimate=g_width*(68.f/1920.f);job.wLo=std::max(32,(int)(estimate*.70f));job.wLo=(job.wLo/2)*2;int wHi=std::min(192,(int)(estimate*1.35f));job.wCount=std::max(1,(wHi-job.wLo)/2+1);
            job.xCount=std::max(1,(job.xHi-job.xLo)/4+1);job.total=(size_t)job.wCount*job.xCount*job.yCount;g_jobs.push_back(std::move(job));
        }stats.jobs=(int)g_jobs.size();
    }
    ULONGLONG budgetStart=GetTickCount64();
    while(g_job<g_jobs.size()){
        auto& job=g_jobs[g_job];
        while(job.cursor<job.total){size_t i=job.cursor++;int xIndex=(int)(i%job.xCount);i/=job.xCount;int yIndex=(int)(i%job.yCount);int wIndex=(int)(i/job.yCount);
            int x=job.xLo+xIndex*4,y=yIndex*2,w=job.wLo+wIndex*2,h=(int)(w*9.f/16.f+.5f);float score=Score(job,x,y,w,h);
            if(score>job.best.confidence)job.best={(float)x,(float)y,(float)w,(float)h,score};
            if((job.cursor&31)==0&&GetTickCount64()-budgetStart>=4)return;
        }
        if(!job.refining)PrepareRefinement(job);
        const size_t refinementTotal=job.scales.size()*9*5;
        while(job.refineCursor<refinementTotal){size_t index=job.refineCursor++;int xx=(int)(index%9)-4;index/=9;int yy=(int)(index%5)-2;size_t scale=index/5;
            const auto& t=job.scales[scale];int x=job.seedX+xx,y=job.seedY+yy;float score=FullScore(t,x,y);
            if(score>job.best.confidence)job.best={(float)x,(float)y,(float)t.w,(float)t.h,score};
            if(GetTickCount64()-budgetStart>=4)return;
        }
        stats.best=std::max(stats.best,job.best.confidence);
        if(job.best.confidence>=.80f){
            auto old=g_anchors.find(job.key);
            // A large jump needs strong evidence, not a weak alias elsewhere in the ROI.
            bool accept=old==g_anchors.end()||fabsf(old->second.value.x-job.best.x)<12.f||job.best.confidence>=.90f;
            if(accept){g_anchors[job.key]={job.best,now};++stats.matched;}
        } // Keep last confirmed geometry up to 30 s through death tint/short occlusion.
        ++g_job;
        if(GetTickCount64()-budgetStart>=4)return;
    }
}
bool Find(const FrameUnit& unit,Anchor& anchor) {
    auto it=g_anchors.find(Key(unit));if(it==g_anchors.end()||!g_width||!g_bufferHeight)return false;
    if(GetTickCount64()-it->second.verifiedAt>30000)return false;
    anchor=it->second.value;float sx=ImGui::GetIO().DisplaySize.x/g_width,sy=ImGui::GetIO().DisplaySize.y/g_bufferHeight;
    anchor.x*=sx;anchor.y*=sy;anchor.w*=sx;anchor.h*=sy;return true;
}
}
