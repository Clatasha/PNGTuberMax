#pragma once
#include "imgui.h"
// Shared by frame-level focus handling and event-level avatar tools.
namespace MaxPreviewInput {
inline bool PopupOpen(){return ImGui::IsPopupOpen("",ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel);}
inline bool PointerOwnedByUi(){return PopupOpen()||ImGui::GetIO().WantCaptureMouse||ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);}
inline bool SceneClick(float x,float y,float w,float h){
 auto& io=ImGui::GetIO();
 if(!ImGui::IsMouseClicked(0)||PointerOwnedByUi()||io.MousePos.x<=x||io.MousePos.x>=x+w||io.MousePos.y<=y||io.MousePos.y>=y+h)return false;
 ImGui::SetWindowFocus(nullptr);return true;
}
}
