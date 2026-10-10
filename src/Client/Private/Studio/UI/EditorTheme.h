#pragma once

#include "imgui.h"

namespace studio::ui
{
    inline void ApplyEditorTheme(const float scale)
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 3.0f;
        style.ChildRounding = 3.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 5.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;

        style.WindowPadding = ImVec2(10, 8);
        style.FramePadding = ImVec2(9, 6);
        style.CellPadding = ImVec2(8, 6);

        style.ItemSpacing = ImVec2(8, 7);
        style.ItemInnerSpacing = ImVec2(7, 5);
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 10.0f;

        style.DisabledAlpha = 0.48f;

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] =
            ImVec4(0.88f, 0.90f, 0.93f, 1.0f);

        colors[ImGuiCol_TextDisabled] =
            ImVec4(0.48f, 0.52f, 0.57f, 1.0f);

        colors[ImGuiCol_WindowBg] =
            ImVec4(0.065f, 0.071f, 0.081f, 1.0f);

        colors[ImGuiCol_ChildBg] =
            ImVec4(0.080f, 0.087f, 0.098f, 1.0f);

        colors[ImGuiCol_PopupBg] =
            ImVec4(0.095f, 0.103f, 0.117f, 1.0f);

        colors[ImGuiCol_Border] =
            ImVec4(0.19f, 0.21f, 0.24f, 0.85f);

        colors[ImGuiCol_FrameBg] =
            ImVec4(0.13f, 0.14f, 0.16f, 1.0f);

        colors[ImGuiCol_FrameBgHovered] =
            ImVec4(0.20f, 0.25f, 0.30f, 1.0f);

        colors[ImGuiCol_FrameBgActive] =
            ImVec4(0.18f, 0.31f, 0.42f, 1.0f);

        colors[ImGuiCol_TitleBg] =
            ImVec4(0.085f, 0.092f, 0.105f, 1.0f);

        colors[ImGuiCol_TitleBgActive] =
            ImVec4(0.11f, 0.12f, 0.14f, 1.0f);

        colors[ImGuiCol_MenuBarBg] =
            ImVec4(0.095f, 0.103f, 0.116f, 1.0f);

        colors[ImGuiCol_Button] =
            ImVec4(0.16f, 0.18f, 0.20f, 1.0f);

        colors[ImGuiCol_ButtonHovered] =
            ImVec4(0.22f, 0.36f, 0.47f, 1.0f);

        colors[ImGuiCol_ButtonActive] =
            ImVec4(0.17f, 0.43f, 0.62f, 1.0f);

        colors[ImGuiCol_Header] =
            ImVec4(0.14f, 0.23f, 0.31f, 1.0f);

        colors[ImGuiCol_HeaderHovered] =
            ImVec4(0.21f, 0.38f, 0.51f, 1.0f);

        colors[ImGuiCol_HeaderActive] =
            ImVec4(0.19f, 0.43f, 0.61f, 1.0f);

        colors[ImGuiCol_Tab] =
            ImVec4(0.10f, 0.11f, 0.13f, 1.0f);

        colors[ImGuiCol_TabHovered] =
            ImVec4(0.19f, 0.31f, 0.41f, 1.0f);

        colors[ImGuiCol_TabSelected] =
            ImVec4(0.18f, 0.23f, 0.28f, 1.0f);

        colors[ImGuiCol_Separator] =
            ImVec4(0.21f, 0.23f, 0.26f, 1.0f);

        colors[ImGuiCol_SeparatorHovered] =
            ImVec4(0.25f, 0.45f, 0.62f, 1.0f);

        colors[ImGuiCol_SeparatorActive] =
            ImVec4(0.25f, 0.52f, 0.72f, 1.0f);

        colors[ImGuiCol_CheckMark] =
            ImVec4(0.34f, 0.67f, 0.89f, 1.0f);

        colors[ImGuiCol_SliderGrab] =
            ImVec4(0.31f, 0.52f, 0.68f, 1.0f);

        colors[ImGuiCol_SliderGrabActive] =
            ImVec4(0.38f, 0.69f, 0.90f, 1.0f);

        colors[ImGuiCol_DockingPreview] =
            ImVec4(0.24f, 0.52f, 0.76f, 0.48f);

        colors[ImGuiCol_DockingEmptyBg] =
            ImVec4(0.045f, 0.050f, 0.060f, 1.0f);

        colors[ImGuiCol_ModalWindowDimBg] =
            ImVec4(0.0f, 0.0f, 0.0f, 0.65f);

        style.ScaleAllSizes(scale);
    }
}