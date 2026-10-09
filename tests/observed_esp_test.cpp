#include "observed_esp.h"
#include "esp_projection.h"
#include <cassert>
#include <iostream>
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(1280,720);io.DeltaTime=1.f/60.f;
 io.Fonts->AddFontDefault();unsigned char* pixels=nullptr;int w=0,h=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);io.Fonts->SetTexID((ImTextureID)1);
 ImGui::NewFrame();auto* dl=ImGui::GetBackgroundDrawList();
 Frame frame;frame.ok=false;frame.observedOnly=true;
 FrameUnit hero;hero.kind=UnitKind::Hero;hero.team=3;hero.hp=60;hero.maxHp=100;hero.alive=true;hero.pos={10,20,0};hero.hbOffset=200;strcpy(hero.nick,"Tidehunter");frame.units={hero};
 auto project=[](const Vec3& pos,ImVec2& out){out=ImVec2(640+pos.x*.1f,500-pos.z*.5f-pos.y*.1f);return true;};
 int heroCalls=0,wardCalls=0,roshanCalls=0;
 auto render=[&](const Frame& f,auto project,bool heroes,bool wards,bool roshan,bool boxes){
     return observedesp::Draw(f,dl,project,1280,720,heroes,wards,roshan,
       [&](const FrameUnit& u){++heroCalls;if(boxes)observedesp::HeroBox(u,dl,project,720,IM_COL32(168,85,247,255));},
       [&](const FrameUnit&){++wardCalls;},[&](const FrameUnit&){++roshanCalls;});
 };
 int before=dl->VtxBuffer.Size;auto stats=render(frame,project,true,true,true,true);
 assert(stats.considered==1&&stats.projected==1&&stats.drawn==1&&dl->VtxBuffer.Size>before);
 frame.observedOnly=false;before=dl->VtxBuffer.Size;stats=render(frame,project,true,true,true,true);assert(stats.drawn==0&&dl->VtxBuffer.Size==before);
 frame.observedOnly=true;stats=render(frame,project,false,true,true,true);assert(stats.drawn==0);
 auto noProject=[](const Vec3&,ImVec2&){return false;};stats=render(frame,noProject,true,true,true,true);assert(stats.considered==1&&stats.projected==0&&stats.drawn==0);
 frame.units[0].alive=false;stats=render(frame,project,true,true,true,true);assert(stats.drawn==0);
 frame.units[0].alive=true;frame.units[0].hp=101;stats=render(frame,project,true,true,true,true);assert(stats.drawn==0);
 frame.units[0].hp=60;auto inverted=[](const Vec3& pos,ImVec2& out){out=ImVec2(640,200+pos.z*.5f);return true;};stats=render(frame,inverted,true,true,true,true);assert(stats.projected==1&&stats.drawn==0);
 frame.units[0].hp=60;before=dl->VtxBuffer.Size;
 assert(observedesp::HeroBox(frame.units[0],dl,project,720,IM_COL32_WHITE)&&dl->VtxBuffer.Size>before);
 assert(!observedesp::HeroBox(frame.units[0],dl,noProject,720,IM_COL32_WHITE));
 // Regression: the user's exact ESP2 matrix with an off-screen zero-origin hero FIRST.
 const float matrix[16]={.865815f,1.8923e-8f,3.27756e-8f,1828.6f,
 -5.82677e-8f,1.33301f,.769613f,-1888.88f,
 2.18876e-8f,.50073f,-.86729f,631.43f,
 2.18557e-8f,.5f,-.866025f,637.51f};
 Frame reported;reported.observedOnly=true;
 FrameUnit hidden=hero;hidden.pos={0,0,0};FrameUnit visible=hero;
 // Synthetic camera-centred point; NOT a claim about the actual hero's live coordinates.
 visible.pos={-1828.6f/.865815f,1888.88f/1.33301f,0};reported.units={hidden,visible};
 auto choice=espprojection::Resolve(matrix,1280,720,reported,1);
 assert(choice.mode==0&&choice.uprightVotes==1&&choice.flippedVotes==0);
 float fx,fy,fw,hx,hy,hw;
 assert(espprojection::Project(matrix,1,hidden.pos,1280,720,fx,fy,fw));
 Vec3 hiddenTop{0,0,150};assert(espprojection::Project(matrix,0,hidden.pos,1280,720,fx,fy,fw));
 assert(espprojection::Project(matrix,0,hiddenTop,1280,720,hx,hy,hw)&&hy>fy); // Old heuristic wrongly flips Y.
 auto realProject=[&](const Vec3& pos,ImVec2& out){float w;return espprojection::Project(matrix,choice.mode,pos,1280,720,out.x,out.y,w);};
 before=dl->VtxBuffer.Size;auto actual=render(reported,realProject,true,true,true,true);
 assert(actual.drawn==1&&dl->VtxBuffer.Size>before);
 float transpose[16];for(int row=0;row<4;++row)for(int col=0;col<4;++col)transpose[row*4+col]=matrix[col*4+row];
 auto transChoice=espprojection::Resolve(transpose,1280,720,reported);assert(transChoice.mode==2&&transChoice.uprightVotes==1);
 reported.units={hidden};choice=espprojection::Resolve(matrix,1280,720,reported,1);assert(choice.mode==0&&choice.uprightVotes==0&&choice.flippedVotes==0);
 reported.units={visible};float invertedM[16];memcpy(invertedM,matrix,sizeof(matrix));for(int i=4;i<8;++i)invertedM[i]=-invertedM[i];
 choice=espprojection::Resolve(invertedM,1280,720,reported);assert(choice.mode==1&&choice.flippedVotes==1);
 Vec3 invalid{NAN,0,0};assert(!espprojection::Project(matrix,0,invalid,1280,720,fx,fy,fw));
 std::cout<<"PASS ESP3 exact reported-matrix regression: off-camera zero origin cannot flip Y, on-camera drawing, transpose, no votes, inverted camera and NaN guards.\n";
 before=dl->VtxBuffer.Size;int callsBefore=heroCalls;
 stats=render(frame,project,true,true,true,false);assert(stats.drawn==1&&heroCalls==callsBefore+1&&dl->VtxBuffer.Size==before);
 FrameUnit ward=hero;ward.kind=UnitKind::Ward;FrameUnit roshan=hero;roshan.kind=UnitKind::Roshan;frame.units={ward,roshan};
 stats=render(frame,project,true,true,true,false);assert(stats.drawn==2&&wardCalls==1&&roshanCalls==1);
 std::cout<<"PASS ESP4 existing-renderer routing, original corner-box vertices, disabled-box toggle emits no diagnostic bars/text, and ward/Roshan dispatch.\n";
 ImGui::EndFrame();ImGui::DestroyContext();
 std::cout<<"PASS observed ESP dispatcher and corner-box helper: invalid-local-frame drawing emits ImGui vertices; config/dead/HP/projection/geometry guards. No GPU or live-game test.\n";
}
