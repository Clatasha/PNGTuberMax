#include "../RahiTuber/MaxUi.h"
void CompileControls(sf::Texture& texture){MaxUi::Button("Save",ImVec2(100,30));MaxUi::IconButton("save",texture,sf::Vector2f(24,24),sf::Color::Transparent,sf::Color::White);MaxUi::ApplyStyle(ImGui::GetStyle(),1);MaxUi::BrandHeader();}
