// Original PNGTuber Max interface components by Clatasha, 2026.
#pragma once
#include <utility>
#include <algorithm>
#include <string>
#include <SFML/Graphics/Texture.hpp>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui-SFML.h"
namespace MaxUi {
inline void FinishRaised(ImDrawList* draw, ImDrawListSplitter& channels) {
    const auto a=ImGui::GetItemRectMin(), b=ImGui::GetItemRectMax();
    if(!ImGui::IsItemVisible()){channels.Merge(draw);return;}
    const bool held=ImGui::IsItemActive();const float offset=held?1.f:3.f;
    channels.SetCurrentChannel(draw,0);
    draw->AddRectFilled(ImVec2(a.x+1,a.y+offset),ImVec2(b.x+1,b.y+offset+2),IM_COL32(0,4,12,100),6.f);
    channels.SetCurrentChannel(draw,1);
    draw->AddLine(ImVec2(a.x+6,a.y+1),ImVec2(b.x-6,a.y+1),IM_COL32(170,212,255,held?18:58));
    draw->AddLine(ImVec2(a.x+6,b.y-1),ImVec2(b.x-6,b.y-1),IM_COL32(1,7,20,120));
    channels.Merge(draw);
}
inline bool Button(const char* label,const ImVec2& size=ImVec2(0,0)) {
    auto* draw=ImGui::GetWindowDrawList();ImDrawListSplitter channels;channels.Split(draw,2);channels.SetCurrentChannel(draw,1);
    bool hit=ImGui::Button(label,size);FinishRaised(draw,channels);return hit;
}
// Camera action colours remain consistent in the toolbar and preferences.
inline bool CameraButton(bool running,const ImVec2& size=ImVec2(0,0),const char* label=nullptr) {
    ImGui::PushStyleColor(ImGuiCol_Button,running?ImVec4(.58f,.12f,.15f,1):ImVec4(.04f,.36f,.18f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,running?ImVec4(.72f,.16f,.20f,1):ImVec4(.05f,.48f,.24f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,running?ImVec4(.46f,.09f,.12f,1):ImVec4(.03f,.29f,.15f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.96f,1.f,.98f,1));
    bool hit=Button(label?label:(running?"Stop virtual camera":"Start virtual camera"),size);
    ImGui::PopStyleColor(4);return hit;
}
inline void CameraState(bool running) {
    ImGui::TextColored(running?ImVec4(.37f,.94f,.62f,1):ImVec4(.95f,.78f,.42f,1),"%s",running?"CAMERA OUTPUT ON":"CAMERA OUTPUT OFF");
    if(!running)ImGui::TextWrapped("Press Start virtual camera to send your avatar.");
}
template<typename... Args> inline bool IconButton(Args&&... args) {
    auto* draw=ImGui::GetWindowDrawList();ImDrawListSplitter channels;channels.Split(draw,2);channels.SetCurrentChannel(draw,1);
    bool hit=ImGui::ImageButton(std::forward<Args>(args)...);FinishRaised(draw,channels);return hit;
}
// Explicit overload accepts the brace-initialized sizes used by inherited controls.
inline bool IconButton(const char* id,const sf::Texture& texture,const sf::Vector2f& size,const sf::Color& bg=sf::Color::Transparent,const sf::Color& tint=sf::Color::White) {
    auto* draw=ImGui::GetWindowDrawList();ImDrawListSplitter channels;channels.Split(draw,2);channels.SetCurrentChannel(draw,1);
    bool hit=ImGui::ImageButton(id,texture,size,bg,tint);FinishRaised(draw,channels);return hit;
}
inline bool IconLabelButton(const char* id,const char* label,const sf::Texture* icon,float iconSize,bool compact=false) {
 if(compact){
  float width=ImGui::GetContentRegionAvail().x,font=ImGui::GetFontSize(),height=std::max(iconSize,font)+ImGui::GetStyle().FramePadding.y*2;
  bool pressed=Button((std::string("##")+id).c_str(),{width,height});auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();auto* draw=ImGui::GetWindowDrawList();auto text=ImGui::CalcTextSize(label);float gap=ImGui::GetStyle().ItemInnerSpacing.x;
  bool showIcon=icon&&iconSize+gap+text.x<=width-ImGui::GetStyle().FramePadding.x*2;float block=text.x+(showIcon?iconSize+gap:0),x=a.x+(width-block)*.5f;
  draw->PushClipRect(a,b,true);if(showIcon){draw->AddImage(static_cast<ImTextureID>(icon->getNativeHandle()),{x,a.y+(height-iconSize)*.5f},{x+iconSize,a.y+(height+iconSize)*.5f});x+=iconSize+gap;}draw->AddText({x,a.y+(height-text.y)*.5f},ImGui::GetColorU32(ImGuiCol_Text),label);draw->PopClipRect();return pressed;
 }
 const float width=ImGui::GetContentRegionAvail().x,font=ImGui::GetFontSize(),height=iconSize+font+12;
 bool pressed=Button((std::string("##")+id).c_str(),{width,height});auto a=ImGui::GetItemRectMin();auto* draw=ImGui::GetWindowDrawList();
 if(icon)draw->AddImage(static_cast<ImTextureID>(icon->getNativeHandle()),{a.x+(width-iconSize)*.5f,a.y+4},{a.x+(width+iconSize)*.5f,a.y+4+iconSize});
 auto text=ImGui::CalcTextSize(label);draw->AddText({a.x+(width-text.x)*.5f,a.y+iconSize+7},ImGui::GetColorU32(ImGuiCol_Text),label);return pressed;
}
inline void ApplyStyle(ImGuiStyle& s,float scale) {
    s.WindowRounding=14*scale;s.ChildRounding=10*scale;s.PopupRounding=10*scale;s.FrameRounding=6*scale;s.GrabRounding=5*scale;s.TabRounding=6*scale;
    s.WindowBorderSize=1;s.ChildBorderSize=1;s.PopupBorderSize=1;s.FrameBorderSize=1;
    s.WindowPadding=ImVec2(14*scale,12*scale);s.FramePadding=ImVec2(6*scale,5*scale);s.ItemSpacing=ImVec2(7*scale,6*scale);s.ItemInnerSpacing=ImVec2(6*scale,5*scale);
    s.Colors[ImGuiCol_WindowBg]=ImVec4(.035f,.055f,.095f,1);
    s.Colors[ImGuiCol_ChildBg]=ImVec4(.065f,.10f,.17f,1);s.Colors[ImGuiCol_PopupBg]=ImVec4(.07f,.115f,.19f,1);
    s.Colors[ImGuiCol_Text]=ImVec4(.91f,.95f,1,1);s.Colors[ImGuiCol_TextDisabled]=ImVec4(.47f,.57f,.70f,1);
    s.Colors[ImGuiCol_Border]=ImVec4(.18f,.29f,.43f,.8f);s.Colors[ImGuiCol_BorderShadow]=ImVec4(0,.015f,.035f,.6f);
    s.Colors[ImGuiCol_FrameBg]=ImVec4(.055f,.095f,.16f,1);s.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.10f,.19f,.31f,1);s.Colors[ImGuiCol_FrameBgActive]=ImVec4(.10f,.24f,.40f,1);
    s.Colors[ImGuiCol_Button]=ImVec4(.12f,.21f,.34f,1);s.Colors[ImGuiCol_ButtonHovered]=ImVec4(.17f,.35f,.57f,1);s.Colors[ImGuiCol_ButtonActive]=ImVec4(.11f,.30f,.52f,1);
    s.Colors[ImGuiCol_Header]=ImVec4(.11f,.19f,.31f,1);s.Colors[ImGuiCol_HeaderHovered]=ImVec4(.15f,.29f,.46f,1);s.Colors[ImGuiCol_HeaderActive]=ImVec4(.16f,.34f,.55f,1);
    s.Colors[ImGuiCol_TitleBg]=ImVec4(.045f,.075f,.13f,1);s.Colors[ImGuiCol_TitleBgActive]=ImVec4(.09f,.16f,.27f,1);s.Colors[ImGuiCol_TitleBgCollapsed]=s.Colors[ImGuiCol_TitleBg];
    s.Colors[ImGuiCol_Tab]=ImVec4(.08f,.14f,.24f,1);s.Colors[ImGuiCol_TabHovered]=ImVec4(.17f,.35f,.57f,1);s.Colors[ImGuiCol_TabSelected]=ImVec4(.13f,.28f,.46f,1);
    s.Colors[ImGuiCol_CheckMark]=s.Colors[ImGuiCol_SliderGrab]=ImVec4(.28f,.65f,1,1);s.Colors[ImGuiCol_SliderGrabActive]=ImVec4(.58f,.81f,1,1);
    s.Colors[ImGuiCol_Separator]=ImVec4(.19f,.31f,.46f,.65f);s.Colors[ImGuiCol_ScrollbarBg]=ImVec4(.04f,.07f,.12f,1);s.Colors[ImGuiCol_ScrollbarGrab]=ImVec4(.17f,.28f,.43f,1);
}
inline void BrandHeader() {
    auto* draw=ImGui::GetWindowDrawList();const auto a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=58;
    draw->AddRectFilled(ImVec2(a.x+2,a.y+4),ImVec2(a.x+w+2,a.y+h+4),IM_COL32(0,4,12,130),9);
    draw->AddRectFilled(a,ImVec2(a.x+w,a.y+h),IM_COL32(26,47,78,255),9);
    draw->AddLine(ImVec2(a.x+9,a.y+1),ImVec2(a.x+w-9,a.y+1),IM_COL32(150,204,255,70));
    draw->AddRectFilled(ImVec2(a.x+12,a.y+12),ImVec2(a.x+16,a.y+h-12),IM_COL32(67,155,255,255),2);
    draw->AddText(ImVec2(a.x+28,a.y+10),IM_COL32(235,245,255,255),"PNGTUBER MAX");
    draw->AddText(ImVec2(a.x+28,a.y+32),IM_COL32(147,182,220,255),"by Clatasha  /  Avatar Studio");
    ImGui::Dummy(ImVec2(w,h+7));
}
}
