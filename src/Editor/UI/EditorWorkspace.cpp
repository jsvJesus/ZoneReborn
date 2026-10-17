#include "Editor/UI/EditorWorkspace.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace editor::ui {

    void apply_editor_theme() {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(10.0f, 7.0f);
        style.CellPadding = ImVec2(8.0f, 6.0f);
        style.ItemSpacing = ImVec2(9.0f, 9.0f);
        style.ItemInnerSpacing = ImVec2(7.0f, 6.0f);

        style.WindowRounding = 7.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 5.0f;

        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        style.ScrollbarSize = 11.0f;
        style.GrabMinSize = 10.0f;
        style.IndentSpacing = 16.0f;

        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] =
            ImVec4(0.91f, 0.94f, 0.94f, 1.00f);

        colors[ImGuiCol_TextDisabled] =
            ImVec4(0.52f, 0.58f, 0.60f, 1.00f);

        colors[ImGuiCol_WindowBg] =
            ImVec4(0.080f, 0.095f, 0.105f, 1.00f);

        colors[ImGuiCol_ChildBg] =
            ImVec4(0.105f, 0.125f, 0.135f, 1.00f);

        colors[ImGuiCol_PopupBg] =
            ImVec4(0.105f, 0.125f, 0.135f, 0.98f);

        colors[ImGuiCol_Border] =
            ImVec4(0.22f, 0.27f, 0.28f, 0.60f);

        colors[ImGuiCol_FrameBg] =
            ImVec4(0.15f, 0.18f, 0.19f, 1.00f);

        colors[ImGuiCol_FrameBgHovered] =
            ImVec4(0.20f, 0.27f, 0.28f, 1.00f);

        colors[ImGuiCol_FrameBgActive] =
            ImVec4(0.22f, 0.34f, 0.33f, 1.00f);

        colors[ImGuiCol_TitleBg] =
            ImVec4(0.07f, 0.09f, 0.10f, 1.00f);

        colors[ImGuiCol_TitleBgActive] =
            ImVec4(0.11f, 0.15f, 0.16f, 1.00f);

        colors[ImGuiCol_MenuBarBg] =
            ImVec4(0.10f, 0.13f, 0.14f, 1.00f);

        colors[ImGuiCol_Button] =
            ImVec4(0.16f, 0.29f, 0.28f, 1.00f);

        colors[ImGuiCol_ButtonHovered] =
            ImVec4(0.20f, 0.40f, 0.37f, 1.00f);

        colors[ImGuiCol_ButtonActive] =
            ImVec4(0.23f, 0.49f, 0.44f, 1.00f);

        colors[ImGuiCol_Header] =
            ImVec4(0.17f, 0.27f, 0.27f, 1.00f);

        colors[ImGuiCol_HeaderHovered] =
            ImVec4(0.22f, 0.38f, 0.36f, 1.00f);

        colors[ImGuiCol_HeaderActive] =
            ImVec4(0.24f, 0.45f, 0.41f, 1.00f);

        colors[ImGuiCol_CheckMark] =
            ImVec4(0.38f, 0.85f, 0.74f, 1.00f);

        colors[ImGuiCol_SliderGrab] =
            ImVec4(0.33f, 0.72f, 0.63f, 1.00f);

        colors[ImGuiCol_SliderGrabActive] =
            ImVec4(0.43f, 0.89f, 0.78f, 1.00f);

        colors[ImGuiCol_Separator] =
            ImVec4(0.20f, 0.26f, 0.27f, 1.00f);

        colors[ImGuiCol_ResizeGrip] =
            ImVec4(0.25f, 0.52f, 0.46f, 0.25f);

        colors[ImGuiCol_ResizeGripHovered] =
            ImVec4(0.35f, 0.72f, 0.64f, 0.70f);

        colors[ImGuiCol_ResizeGripActive] =
            ImVec4(0.40f, 0.84f, 0.72f, 1.00f);

        colors[ImGuiCol_Tab] =
            ImVec4(0.12f, 0.16f, 0.17f, 1.00f);

        colors[ImGuiCol_TabHovered] =
            ImVec4(0.22f, 0.36f, 0.34f, 1.00f);

        colors[ImGuiCol_TabSelected] =
            ImVec4(0.19f, 0.31f, 0.30f, 1.00f);

        colors[ImGuiCol_DockingPreview] =
            ImVec4(0.35f, 0.76f, 0.65f, 0.65f);

        colors[ImGuiCol_DockingEmptyBg] =
            ImVec4(0.065f, 0.080f, 0.085f, 1.00f);
    }

    void draw_editor_workspace(bool& resetLayout) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(
            0,
            viewport,
            ImGuiDockNodeFlags_None
        );

        if (!resetLayout) {
            return;
        }

        resetLayout = false;

        ImGui::DockBuilderRemoveNode(dockspaceId);

        ImGui::DockBuilderAddNode(
            dockspaceId,
            ImGuiDockNodeFlags_DockSpace
        );

        ImGui::DockBuilderSetNodeSize(
            dockspaceId,
            viewport->Size
        );

        ImGuiID center = dockspaceId;
        ImGuiID left = 0;
        ImGuiID right = 0;
        ImGuiID bottom = 0;

        ImGui::DockBuilderSplitNode( // #1
            center,
            ImGuiDir_Left,
            0.24f,
            &left,
            &center
        );

        ImGui::DockBuilderSplitNode( // #2
            center,
            ImGuiDir_Right,
            0.25f,
            &right,
            &center
        );

        ImGui::DockBuilderSplitNode( // #3
            center,
            ImGuiDir_Down,
            0.22f,
            &bottom,
            &center
        );

        ImGuiID inspector = 0;

        ImGui::DockBuilderSplitNode( // #4
            right,
            ImGuiDir_Down,
            0.50f,
            &inspector,
            &right
        );

        ImGui::DockBuilderDockWindow(
            "Terrain Tools",
            left
        );

        ImGui::DockBuilderDockWindow(
            "Map Layout",
            left
        );

        ImGui::DockBuilderDockWindow(
            "World Outliner",
            right
        );

        ImGui::DockBuilderDockWindow(
            "Map Inspector",
            inspector
        );

        ImGui::DockBuilderDockWindow(
            "World Storage",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Terrain Viewport",
            center
        );

        ImGui::DockBuilderFinish(dockspaceId);
    }

}
