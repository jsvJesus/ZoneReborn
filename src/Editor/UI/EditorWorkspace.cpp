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

        style.WindowRounding = 0.0f;
        style.ChildRounding = 2.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 3.0f;

        style.WindowBorderSize = 1.0f;
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
            ImVec4(0.55f, 0.59f, 0.65f, 1.00f);

        colors[ImGuiCol_WindowBg] =
            ImVec4(0.075f, 0.082f, 0.094f, 1.00f);

        colors[ImGuiCol_ChildBg] =
            ImVec4(0.095f, 0.105f, 0.122f, 1.00f);

        colors[ImGuiCol_PopupBg] =
            ImVec4(0.105f, 0.115f, 0.135f, 0.98f);

        colors[ImGuiCol_Border] =
            ImVec4(0.23f, 0.25f, 0.29f, 0.80f);

        colors[ImGuiCol_FrameBg] =
            ImVec4(0.14f, 0.15f, 0.18f, 1.00f);

        colors[ImGuiCol_FrameBgHovered] =
            ImVec4(0.20f, 0.25f, 0.33f, 1.00f);

        colors[ImGuiCol_FrameBgActive] =
            ImVec4(0.20f, 0.34f, 0.52f, 1.00f);

        colors[ImGuiCol_TitleBg] =
            ImVec4(0.07f, 0.09f, 0.10f, 1.00f);

        colors[ImGuiCol_TitleBgActive] =
            ImVec4(0.12f, 0.18f, 0.26f, 1.00f);

        colors[ImGuiCol_MenuBarBg] =
            ImVec4(0.105f, 0.115f, 0.135f, 1.00f);

        colors[ImGuiCol_Button] =
            ImVec4(0.17f, 0.20f, 0.25f, 1.00f);

        colors[ImGuiCol_ButtonHovered] =
            ImVec4(0.23f, 0.32f, 0.43f, 1.00f);

        colors[ImGuiCol_ButtonActive] =
            ImVec4(0.23f, 0.39f, 0.58f, 1.00f);

        colors[ImGuiCol_Header] =
            ImVec4(0.17f, 0.21f, 0.27f, 1.00f);

        colors[ImGuiCol_HeaderHovered] =
            ImVec4(0.22f, 0.31f, 0.42f, 1.00f);

        colors[ImGuiCol_HeaderActive] =
            ImVec4(0.23f, 0.38f, 0.54f, 1.00f);

        colors[ImGuiCol_CheckMark] =
            ImVec4(0.35f, 0.67f, 0.98f, 1.00f);

        colors[ImGuiCol_SliderGrab] =
            ImVec4(0.32f, 0.60f, 0.89f, 1.00f);

        colors[ImGuiCol_SliderGrabActive] =
            ImVec4(0.42f, 0.70f, 1.00f, 1.00f);

        colors[ImGuiCol_Separator] =
            ImVec4(0.20f, 0.25f, 0.32f, 1.00f);

        colors[ImGuiCol_ResizeGrip] =
            ImVec4(0.25f, 0.48f, 0.72f, 0.25f);

        colors[ImGuiCol_ResizeGripHovered] =
            ImVec4(0.35f, 0.62f, 0.91f, 0.70f);

        colors[ImGuiCol_ResizeGripActive] =
            ImVec4(0.40f, 0.69f, 1.00f, 1.00f);

        colors[ImGuiCol_Tab] =
            ImVec4(0.11f, 0.12f, 0.14f, 1.00f);

        colors[ImGuiCol_TabHovered] =
            ImVec4(0.20f, 0.29f, 0.40f, 1.00f);

        colors[ImGuiCol_TabSelected] =
            ImVec4(0.17f, 0.22f, 0.29f, 1.00f);

        colors[ImGuiCol_DockingPreview] =
            ImVec4(0.29f, 0.51f, 0.78f, 0.65f);

        colors[ImGuiCol_DockingEmptyBg] =
            ImVec4(0.055f, 0.060f, 0.070f, 1.00f);
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
        ImGuiID right = 0;
        ImGuiID bottom = 0;

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Right,
            0.27f,
            &right,
            &center
        );

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Down,
            0.26f,
            &bottom,
            &center
        );

        ImGuiID outliner = 0;

        ImGui::DockBuilderSplitNode(
            right,
            ImGuiDir_Up,
            0.44f,
            &outliner,
            &right
        );

        ImGui::DockBuilderDockWindow(
            "Outliner",
            outliner
        );

        ImGui::DockBuilderDockWindow(
            "Details",
            right
        );

        ImGui::DockBuilderDockWindow(
            "World Settings",
            right
        );

        ImGui::DockBuilderDockWindow(
            "World Info",
            right
        );

        ImGui::DockBuilderDockWindow("World Generator", right);
        ImGui::DockBuilderDockWindow("Map Layout", right);
        ImGui::DockBuilderDockWindow("Biome Materials", right);
        ImGui::DockBuilderDockWindow("Vegetation Generator", right);
        ImGui::DockBuilderDockWindow("Terrain Tools", right);

        ImGui::DockBuilderDockWindow(
            "Content Drawer",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Output Log",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Terrain Viewport",
            center
        );

        ImGui::DockBuilderFinish(dockspaceId);
    }

}
