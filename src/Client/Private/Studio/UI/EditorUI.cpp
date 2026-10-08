
#include "Studio/UI/EditorUI.h"

#include "Studio/EditorScene.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <cstdint>
#include <filesystem>

namespace studio::ui
{
    EditorUI::~EditorUI()
    {
        Shutdown();
    }

    bool EditorUI::Initialize(
        HWND window,
        ID3D11Device* device,
        ID3D11DeviceContext* context)
    {
        Shutdown();

        if (!window || !device || !context)
        {
            return false;
        }

        layoutInitialized_ =
            std::filesystem::exists(
                "StudioLayout.ini");

        IMGUI_CHECKVERSION();

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard;

        io.ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        io.IniFilename =
            "StudioLayout.ini";

        ImGui::StyleColorsDark();

        ImGuiStyle& style =
            ImGui::GetStyle();

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;

        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.WindowPadding = ImVec2(8, 8);
        style.FramePadding = ImVec2(7, 5);
        style.ItemSpacing = ImVec2(8, 6);

        style.Colors[ImGuiCol_WindowBg] =
            ImVec4(0.075f, 0.080f, 0.085f, 1.0f);

        style.Colors[ImGuiCol_ChildBg] =
            ImVec4(0.065f, 0.070f, 0.075f, 1.0f);

        style.Colors[ImGuiCol_TitleBg] =
            ImVec4(0.095f, 0.100f, 0.105f, 1.0f);

        style.Colors[ImGuiCol_TitleBgActive] =
            ImVec4(0.125f, 0.130f, 0.135f, 1.0f);

        style.Colors[ImGuiCol_Tab] =
            ImVec4(0.12f, 0.13f, 0.14f, 1.0f);

        style.Colors[ImGuiCol_TabSelected] =
            ImVec4(0.20f, 0.22f, 0.23f, 1.0f);

        style.Colors[ImGuiCol_HeaderHovered] =
            ImVec4(0.30f, 0.28f, 0.17f, 0.65f);

        style.Colors[ImGuiCol_HeaderActive] =
            ImVec4(0.40f, 0.34f, 0.15f, 0.85f);

        style.Colors[ImGuiCol_ButtonHovered] =
            ImVec4(0.34f, 0.30f, 0.17f, 1.0f);

        style.Colors[ImGuiCol_Separator] =
            ImVec4(0.20f, 0.21f, 0.22f, 1.0f);

        if (!ImGui_ImplWin32_Init(window))
        {
            ImGui::DestroyContext();
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, context))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        initialized_ = true;

        return true;
    }

    void EditorUI::Shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();

        initialized_ = false;
    }

    void EditorUI::BeginFrame(
        const EditorScene& scene,
        ID3D11ShaderResourceView* sceneTexture)
    {
        if (!initialized_)
        {
            return;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        BuildMainMenu();
        BuildDockSpace();

        if (showViewport_)
        {
            BuildViewport(sceneTexture);
        }

        if (showOutliner_)
        {
            BuildSceneOutliner(scene);
        }

        if (showDetails_)
        {
            BuildDetails();
        }

        if (showContentBrowser_)
        {
            BuildContentBrowser();
        }

        if (showConsole_)
        {
            BuildConsole();
        }
    }

    void EditorUI::Render()
    {
        if (!initialized_)
        {
            return;
        }

        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(
            ImGui::GetDrawData());
    }

    void EditorUI::BuildMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(
                        "New Empty Scene",
                        "Ctrl+N"))
                {
                    newSceneRequested_ = true;
                }

                ImGui::Separator();

                ImGui::MenuItem(
                    "Open Level...",
                    nullptr,
                    false,
                    false);

                ImGui::MenuItem(
                    "Save",
                    "Ctrl+S",
                    false,
                    false);

                ImGui::MenuItem(
                    "Save As...",
                    nullptr,
                    false,
                    false);

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    exitRequested_ = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem(
                    "Viewport",
                    nullptr,
                    &showViewport_);

                ImGui::MenuItem(
                    "Scene Outliner",
                    nullptr,
                    &showOutliner_);

                ImGui::MenuItem(
                    "Details",
                    nullptr,
                    &showDetails_);

                ImGui::MenuItem(
                    "Content Browser",
                    nullptr,
                    &showContentBrowser_);

                ImGui::MenuItem(
                    "Console",
                    nullptr,
                    &showConsole_);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (ImGui::GetIO().KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_N))
        {
            newSceneRequested_ = true;
        }
    }

    void EditorUI::BuildDockSpace()
    {
        ImGuiID dockspaceId =
            ImGui::DockSpaceOverViewport(
                0,
                ImGui::GetMainViewport(),
                ImGuiDockNodeFlags_None);

        if (!layoutInitialized_)
        {
            BuildDefaultLayout(dockspaceId);
            layoutInitialized_ = true;
        }
    }

    void EditorUI::BuildDefaultLayout(
        unsigned int dockspaceId)
    {
        const ImGuiViewport* viewport =
            ImGui::GetMainViewport();

        ImGui::DockBuilderRemoveNode(
            dockspaceId);

        ImGui::DockBuilderAddNode(
            dockspaceId,
            ImGuiDockNodeFlags_DockSpace);

        ImGui::DockBuilderSetNodeSize(
            dockspaceId,
            viewport->Size);

        ImGuiID center = dockspaceId;

        const ImGuiID left =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Left,
                0.20f,
                nullptr,
                &center);

        const ImGuiID right =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Right,
                0.23f,
                nullptr,
                &center);

        const ImGuiID bottom =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Down,
                0.25f,
                nullptr,
                &center);

        ImGui::DockBuilderDockWindow(
            "Viewport",
            center);

        ImGui::DockBuilderDockWindow(
            "Scene Outliner",
            left);

        ImGui::DockBuilderDockWindow(
            "Details",
            right);

        ImGui::DockBuilderDockWindow(
            "Content Browser",
            bottom);

        ImGui::DockBuilderDockWindow(
            "Console",
            bottom);

        ImGui::DockBuilderFinish(
            dockspaceId);
    }

    void EditorUI::BuildViewport(
        ID3D11ShaderResourceView* sceneTexture)
    {
        if (ImGui::Begin(
                "Viewport",
                &showViewport_,
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse))
        {
            const ImVec2 size =
                ImGui::GetContentRegionAvail();

            if (size.x > 1.0f &&
                size.y > 1.0f &&
                sceneTexture)
            {
                const ImTextureID textureId =
                    static_cast<ImTextureID>(
                        reinterpret_cast<std::uintptr_t>(
                            sceneTexture));

                ImGui::Image(
                    textureId,
                    size);
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildSceneOutliner(
        const EditorScene& scene)
    {
        if (ImGui::Begin(
                "Scene Outliner",
                &showOutliner_))
        {
            ImGui::TextUnformatted(
                scene.Name().c_str());

            ImGui::Separator();

            if (scene.IsEmpty())
            {
                ImGui::TextDisabled(
                    "No objects in the scene.");
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildDetails()
    {
        if (ImGui::Begin(
                "Details",
                &showDetails_))
        {
            ImGui::TextDisabled(
                "No object selected.");
        }

        ImGui::End();
    }

    void EditorUI::BuildContentBrowser()
    {
        if (ImGui::Begin(
                "Content Browser",
                &showContentBrowser_))
        {
            ImGui::TextDisabled(
                "Asset browser is not initialized.");
        }

        ImGui::End();
    }

    void EditorUI::BuildConsole()
    {
        if (ImGui::Begin(
                "Console",
                &showConsole_))
        {
            ImGui::TextDisabled(
                "Editor initialized.");

            ImGui::TextDisabled(
                "Scene: Untitled");
        }

        ImGui::End();
    }

    bool EditorUI::ConsumeNewSceneRequest() noexcept
    {
        const bool requested =
            newSceneRequested_;

        newSceneRequested_ = false;

        return requested;
    }

    bool EditorUI::ConsumeExitRequest() noexcept
    {
        const bool requested =
            exitRequested_;

        exitRequested_ = false;

        return requested;
    }
}