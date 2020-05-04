
#include "imgui.h"

#include "spritesheet.hpp"
#include "utility/stl_helper.hpp"

//========================================
namespace ui
{
    void DrawSpriteButton(ImVec2 _vec2ButtonWH,
                          int const _iButtonBorder,
                          uint32_t const _uTexId,
                          CSpriteSheet::SSpriteCell const &_SpriteCell,
                          bool _bShowInfo)
    {
        ImTextureID const _ImTexId = (ImTextureID)uint64_t(_uTexId);

        //========================================
        ImGui::BeginGroup();
        {
            ImVec2 const _vec2PreCursorPos = ImGui::GetCursorPos();

            // Draw the button
            if (ImGui::Button("##sprite_button", ImVec2(_vec2ButtonWH.x + _iButtonBorder * 2,
                                                        _vec2ButtonWH.y + _iButtonBorder * 2)))
            {

            }

            // Store the 'after-button' position, we need to jump back to this below
            ImVec2 const _vec2PostCursorPos = ImGui::GetCursorPos();

            ImVec2 const _vec2ImagePos = ImVec2(_vec2PreCursorPos.x + _iButtonBorder, _vec2PreCursorPos.y + _iButtonBorder);

            // Draw the sprite
            if (_ImTexId)
            {
                float _fMult = fminf(_vec2ButtonWH.x / _SpriteCell.w, _vec2ButtonWH.y / _SpriteCell.h);

                ImVec2 _vec2ImageWH(_SpriteCell.w * _fMult,
                                    _SpriteCell.h * _fMult);

                ImVec2 const _vec2UV0 = ImVec2(_SpriteCell.m_fMinX,
                                               _SpriteCell.m_fMinY);
                ImVec2 const _vec2UV1 = ImVec2(_SpriteCell.m_fMaxX,
                                               _SpriteCell.m_fMaxY);

                ImGui::SetCursorPos(_vec2ImagePos);
                ImGui::Image(_ImTexId, _vec2ImageWH, _vec2UV0, _vec2UV1);
            }

            //---------- Draw text info
            //========================================
            if (_bShowInfo == true)
            {
                std::string const _sTextInfo = stl_helper::Format("%s", _SpriteCell.m_sName.c_str());

                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + _vec2ButtonWH.x - static_cast<float>(_iButtonBorder * 2));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                {
                    ImVec2 _vec2TextPos(_vec2PreCursorPos.x + 1, _vec2PreCursorPos.y + 1);
                    ImGui::SetCursorPos(_vec2TextPos);
                    ImGui::Text("%s", _sTextInfo.c_str());
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                {
                    ImGui::SetCursorPos(_vec2PreCursorPos);
                    ImGui::Text("%s", _sTextInfo.c_str());
                }
                ImGui::PopStyleColor();
                ImGui::PopTextWrapPos();
            }
            //========================================

            ImGui::SetCursorPos(_vec2PostCursorPos);
        }
        ImGui::EndGroup();
        //========================================

        // Draw tooltip
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            ImGui::Text("%s\n[%d x %d]",
                        _SpriteCell.m_sName.c_str(),
                        _SpriteCell.w,
                        _SpriteCell.h);
            ImGui::Image(_ImTexId,
                         ImVec2((float)_SpriteCell.w, (float)_SpriteCell.h),
                         ImVec2(_SpriteCell.m_fMinX,
                                _SpriteCell.m_fMinY),
                         ImVec2(_SpriteCell.m_fMaxX,
                                _SpriteCell.m_fMaxY));

            ImGui::EndTooltip();
        }
    }

	void SpriteSheetWindow(class CSpriteSheet const& _SpriteSheet, uint32_t const _uTexId)
	{
        static float s_fIconSize = 64.0f;
        static bool s_bShowNames = false;
        ImGui::SliderFloat("Icon Size", &s_fIconSize, 32.0f, 1024.0f);

        ImGui::SameLine();

        ImGui::Checkbox("Show Names", &s_bShowNames);

        ImGui::Separator();

        {
            auto const & _mapSpriteData = _SpriteSheet.GetSpriteData();

            ImVec2 const c_vec2ButtonWH(s_fIconSize, s_fIconSize);

            float _fCurrentFilledW = 0.0f;
            uint32_t _uButtonBorder = 2;

            for (auto& _item : _mapSpriteData)
            {
                ImGui::PushID(_item.first.c_str());

                ImGui::BeginGroup();
                {
                    DrawSpriteButton(c_vec2ButtonWH,
                                     _uButtonBorder,
                                     _uTexId,
                                     _item.second,
                                     s_bShowNames);
                }
                ImGui::EndGroup();


                _fCurrentFilledW += (c_vec2ButtonWH.x + (_uButtonBorder * 2) + ImGui::GetStyle().ItemSpacing.x);

                if (ImGui::GetContentRegionAvailWidth() - _fCurrentFilledW > c_vec2ButtonWH.x)
                {
                    ImGui::SameLine();
                }
                else
                {
                    _fCurrentFilledW = 0.0f;
                }

                ImGui::PopID();
            }
        }
	}
};
//========================================