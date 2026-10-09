#include "draft_advisor_ui.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1120,640};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
 auto* font=io.Fonts->AddFontFromFileTTF("assets/fonts/ui-semibold.otf",15);if(!font)font=io.Fonts->AddFontDefault();unsigned char* px;int W,H;io.Fonts->GetTexDataAsRGBA32(&px,&W,&H);io.Fonts->SetTexID((ImTextureID)1);
 std::ofstream atlas("build/draft15-font.rgba",std::ios::binary);atlas.write((char*)px,W*H*4);atlas.close();ImGui::NewFrame();
 ImGui::SetNextWindowPos({24,24});ImGui::SetNextWindowSize({740,592});ImGui::Begin("ESP17 / hero - lane - partner",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
 draftadvisor::automatic=false;std::array<int,4> picks={3,19,42,39};if(argc>1){draftadvisor::automatic=true;draftadvisor::live={};if(std::string(argv[1])!="waiting"){draftadvisor::live.ok=true;draftadvisor::live.team=3;draftadvisor::live.source=std::string(argv[1])=="match"?1:2;draftadvisor::live.allies=picks;if(std::string(argv[1])=="picked")draftadvisor::live.ownHero=draftadvisor::IndexByName("npc_dota_hero_axe");}}
 draftadvisor::DrawUI(true,picks);auto* dl=ImGui::GetWindowDrawList();ImGui::End();
 ImGui::Render();std::ofstream out("build/draft15-mesh.json");out<<"{\"font\":["<<W<<","<<H<<"],\"textures\":{},\"lists\":[{\"v\":[";
 for(int i=0;i<dl->VtxBuffer.Size;++i){auto v=dl->VtxBuffer[i];if(i)out<<",";out<<"["<<v.pos.x<<","<<v.pos.y<<","<<v.uv.x<<","<<v.uv.y<<","<<v.col<<"]";}
 out<<"],\"idx\":[";for(int i=0;i<dl->IdxBuffer.Size;++i){if(i)out<<",";out<<dl->IdxBuffer[i];}out<<"],\"cmd\":[";
 for(int i=0;i<dl->CmdBuffer.Size;++i){auto c=dl->CmdBuffer[i];if(i)out<<",";out<<"["<<c.IdxOffset<<","<<c.ElemCount<<","<<c.VtxOffset<<","<<c.GetTexID()<<","<<c.ClipRect.x<<","<<c.ClipRect.y<<","<<c.ClipRect.z<<","<<c.ClipRect.w<<"]";}out<<"]}]}";out.close();ImGui::DestroyContext();
 std::cout<<"PASS production draft advisor UI builds and renders.\n";
}
