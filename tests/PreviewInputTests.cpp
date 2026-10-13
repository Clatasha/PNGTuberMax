#include "../RahiTuber/MaxPreviewInput.h"
#include "imgui_internal.h"
#include <iostream>
#include <stdexcept>
#include <string>
struct Result{bool hit=false,leaked=false,cancelled=false;};
static Result Click(const char* name,bool modal,bool color,bool legacy=false,bool floating=false){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1000,700};io.DeltaTime=1.f/60;io.IniFilename=nullptr;io.Fonts->AddFontDefault();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);bool first=true;ImVec2 lo,hi;Result result;
 auto frame=[&](){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({300,700});ImGui::Begin("Studio");if(first&&!floating){ImGui::OpenPopup(name);first=false;}ImGui::SetNextWindowPos({400,180});ImGui::SetNextWindowSize({300,220});bool open=floating?ImGui::Begin(name):modal?ImGui::BeginPopupModal(name):ImGui::BeginPopup(name);if(open){result.hit|=color?ImGui::ColorButton("Color",{.1f,.4f,.4f,1},0,{60,30}):ImGui::Button("Save / apply",{160,30});lo=ImGui::GetItemRectMin();hi=ImGui::GetItemRectMax();if(!floating)ImGui::EndPopup();}if(floating)ImGui::End();ImGui::End();
 if(legacy){if(ImGui::IsMouseClicked(0)&&io.MousePos.x>300&&io.MousePos.x<800&&io.MousePos.y>0&&io.MousePos.y<700){auto before=GImGui->ActiveId;ImGui::SetWindowFocus(nullptr);result.cancelled|=before!=0&&GImGui->ActiveId==0;}}
 else result.leaked|=MaxPreviewInput::SceneClick(300,0,500,700);
 ImGui::Render();};
 frame();frame();io.AddMousePosEvent((lo.x+hi.x)*.5f,(lo.y+hi.y)*.5f);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();ImGui::DestroyContext();return result;
}
int main(){try{
 auto original=Click("Original preview bug",true,false,true);if(original.hit||!original.cancelled)throw std::runtime_error("Regression fixture did not reproduce lost mouse activation");
 for(const char* name:{"Rename Layer","States Setup","Model Options","Sprite Sheet Setup","Offset Setup","Edit Canvas Preset","Window Presets","Help"})for(bool color:{false,true}){auto r=Click(name,true,color);if(!r.hit||r.leaked)throw std::runtime_error(std::string(name)+" mouse input reached the preview");}
 for(const char* name:{"Face crop editor","BGColorEdit","Dropdown"}){auto r=Click(name,false,false);if(!r.hit||r.leaked)throw std::runtime_error(std::string(name)+" non-modal click reached preview");}
 auto floating=Click("Floating properties",false,false,false,true);if(!floating.hit||floating.leaked)throw std::runtime_error("Floating UI over preview lost focus");
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1000,700};io.DeltaTime=1.f/60;io.IniFilename=nullptr;io.Fonts->AddFontDefault();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);bool hit=false;auto frame=[&](){ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({300,700});ImGui::Begin("Studio");ImGui::TextUnformatted("Dock");ImGui::End();hit|=MaxPreviewInput::SceneClick(300,0,500,700);ImGui::Render();};frame();frame();io.AddMousePosEvent(700,300);frame();io.AddMouseButtonEvent(0,true);frame();if(!hit)throw std::runtime_error("Empty avatar canvas no longer accepts clicks");ImGui::DestroyContext();
 std::cout<<"PASS: reproduced original focus cancellation; modal/non-modal/floating UI retain Save/color mouse clicks over preview; empty canvas still accepts clicks\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
