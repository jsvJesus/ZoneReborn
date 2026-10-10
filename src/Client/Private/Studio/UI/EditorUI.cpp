#include "Studio/UI/EditorUI.h"

#include "Studio/EditorScene.h"
#include "Studio/LevelCatalog.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <cstdint>
#include <filesystem>
#include <utility>

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
            return false;

        window_ = window;

        layoutInitialized_ =
            std::filesystem::exists("StudioLayout.ini");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = "StudioLayout.ini";

        ImFont* font = io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\segoeui.ttf",
            18.0f);

        if (font)
            io.FontDefault = font;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();

        style.ScaleAllSizes(1.15f);

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
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

        style.Colors[ImGuiCol_Tab] =
            ImVec4(0.12f, 0.13f, 0.14f, 1.0f);

        style.Colors[ImGuiCol_TabSelected] =
            ImVec4(0.20f, 0.22f, 0.23f, 1.0f);

        style.Colors[ImGuiCol_Separator] =
            ImVec4(0.20f, 0.21f, 0.22f, 1.0f);

        if (!ImGui_ImplWin32_Init(window))
        {
            ImGui::DestroyContext();
            window_ = nullptr;
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, context))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            window_ = nullptr;
            return false;
        }

        initialized_ = true;

        AddConsoleMessage("Editor initialized.");

        return true;
    }

    void EditorUI::Shutdown()
    {
        if (!initialized_)
            return;

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();

        initialized_ = false;
        window_ = nullptr;
    }

    void EditorUI::BeginFrame(
        const EditorScene& scene,
        const LevelCatalog& catalog,
        ID3D11ShaderResourceView* sceneTexture)
    {
        if (!initialized_)
            return;

        viewportHovered_ = false;
        viewportWheel_ = 0.0f;
        viewportRectangle_ = {};

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        BuildMainMenu();
        BuildDockSpace();

        if (showViewport_)
            BuildViewport(sceneTexture);

        if (showOutliner_)
            BuildSceneOutliner(scene);

        if (showDetails_)
            BuildDetails(scene);

        if (showContentBrowser_)
            BuildContentBrowser(catalog);

        if (showConsole_)
            BuildConsole();

        BuildOpenLevelDialog(catalog);
    }

    void EditorUI::Render()
    {
        if (!initialized_)
            return;

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
                    "New Empty Scene", "Ctrl+N"))
                {
                    newSceneRequested_ = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem(
                    "Open Level...", "Ctrl+O"))
                {
                    openDialogRequested_ = true;
                }

                ImGui::MenuItem(
                    "Save", "Ctrl+S", false, false);

                ImGui::MenuItem(
                    "Save As...", nullptr, false, false);

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                    exitRequested_ = true;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem(
                    "Viewport", nullptr, &showViewport_);

                ImGui::MenuItem(
                    "Scene Outliner", nullptr, &showOutliner_);

                ImGui::MenuItem(
                    "Details", nullptr, &showDetails_);

                ImGui::MenuItem(
                    "Content Browser", nullptr, &showContentBrowser_);

                ImGui::MenuItem(
                    "Console", nullptr, &showConsole_);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        const ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl && !io.WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_N))
                newSceneRequested_ = true;

            if (ImGui::IsKeyPressed(ImGuiKey_O))
                openDialogRequested_ = true;
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

        ImGui::DockBuilderRemoveNode(dockspaceId);

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
            "Viewport", center);

        ImGui::DockBuilderDockWindow(
            "Scene Outliner", left);

        ImGui::DockBuilderDockWindow(
            "Details", right);

        ImGui::DockBuilderDockWindow(
            "Content Browser", bottom);

        ImGui::DockBuilderDockWindow(
            "Console", bottom);

        ImGui::DockBuilderFinish(dockspaceId);
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

                ImGui::Image(textureId, size);

                const ImVec2 minimum =
                    ImGui::GetItemRectMin();

                const ImVec2 maximum =
                    ImGui::GetItemRectMax();

                POINT topLeft
                {
                    static_cast<LONG>(minimum.x),
                    static_cast<LONG>(minimum.y)
                };

                POINT bottomRight
                {
                    static_cast<LONG>(maximum.x),
                    static_cast<LONG>(maximum.y)
                };

                ScreenToClient(window_, &topLeft);
                ScreenToClient(window_, &bottomRight);

                viewportRectangle_ =
                {
                    topLeft.x,
                    topLeft.y,
                    bottomRight.x,
                    bottomRight.y
                };

                viewportHovered_ =
                    ImGui::IsItemHovered() &&
                    !ImGui::GetIO().WantTextInput;

                if (viewportHovered_)
                {
                    viewportWheel_ =
                        ImGui::GetIO().MouseWheel;
                }
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildSceneOutliner(
        const EditorScene& scene)
    {
        if (ImGui::Begin(
            "Scene Outliner", &showOutliner_))
        {
            ImGui::TextUnformatted(scene.Name().c_str());
            ImGui::Separator();

            if (scene.IsEmpty())
            {
                ImGui::TextDisabled(
                    "No objects in the scene.");
            }
            else
            {
                if (ImGui::TreeNode("Static geometry"))
                {
                    ImGui::Text(
                        "Meshes: %zu",
                        scene.Statistics().meshes);

                    ImGui::Text(
                        "Visuals: %zu",
                        scene.Statistics().visuals);

                    ImGui::TreePop();
                }
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildDetails(
        const EditorScene& scene)
    {
        if (ImGui::Begin("Details", &showDetails_))
        {
            if (scene.IsEmpty())
            {
                ImGui::TextDisabled(
                    "No object selected.");
            }
            else
            {
                const SceneStatistics& stats =
                    scene.Statistics();

                ImGui::Text(
                    "Level: %s", scene.Name().c_str());

                ImGui::Separator();

                ImGui::Text(
                    "Visuals: %zu", stats.visuals);

                ImGui::Text(
                    "Meshes: %zu", stats.meshes);

                ImGui::Text(
                    "Vertices: %zu", stats.vertices);

                ImGui::Text(
                    "Triangles: %zu", stats.triangles);

                ImGui::Text(
                    "Loaded textures: %zu", stats.textures);

                ImGui::Text(
                    "Missing textures: %zu",
                    stats.missingTextures);

                ImGui::Separator();

                const std::string path =
                    PathToUtf8(scene.Directory());

                ImGui::TextWrapped(
                    "%s", path.c_str());
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildContentBrowser(
        const LevelCatalog& catalog)
    {
        if (ImGui::Begin(
            "Content Browser", &showContentBrowser_))
        {
            if (ImGui::Button("Open Level"))
                openDialogRequested_ = true;

            ImGui::SameLine();

            if (ImGui::Button("Refresh"))
                refreshLevelsRequested_ = true;

            ImGui::Separator();

            ImGui::Text(
                "Levels: %zu", catalog.Entries().size());

            for (const LevelEntry& level : catalog.Entries())
            {
                ImGui::BulletText(
                    "%s", level.name.c_str());
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildConsole()
    {
        if (ImGui::Begin("Console", &showConsole_))
        {
            for (const std::string& line : consoleLines_)
            {
                ImGui::TextWrapped(
                    "%s", line.c_str());
            }

            if (scrollConsole_)
            {
                ImGui::SetScrollHereY(1.0f);
                scrollConsole_ = false;
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildOpenLevelDialog(
        const LevelCatalog& catalog)
    {
        if (openDialogRequested_)
        {
            ImGui::OpenPopup("Open Level");

            selectedLevelIndex_ = -1;
            openDialogRequested_ = false;
        }

        if (!ImGui::BeginPopupModal(
            "Open Level",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        ImGui::TextUnformatted(
            "Select a level from gamedata/levels");

        ImGui::Separator();

        if (catalog.Roots().empty())
        {
            ImGui::TextDisabled(
                "gamedata/levels directory not found.");
        }
        else if (catalog.Entries().empty())
        {
            ImGui::TextDisabled(
                "No valid X-Ray levels found.");
        }

        if (selectedLevelIndex_ >=
            static_cast<int>(catalog.Entries().size()))
        {
            selectedLevelIndex_ = -1;
        }

        ImGui::BeginChild(
            "##level_list",
            ImVec2(580.0f, 320.0f),
            true);

        const auto& levels = catalog.Entries();

        for (std::size_t i = 0; i < levels.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            const bool selected =
                selectedLevelIndex_ == static_cast<int>(i);

            if (ImGui::Selectable(
                levels[i].name.c_str(), selected))
            {
                selectedLevelIndex_ =
                    static_cast<int>(i);

                if (ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left))
                {
                    requestedLevel_ = levels[i].directory;
                    openLevelRequested_ = true;

                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        if (selectedLevelIndex_ >= 0 &&
            selectedLevelIndex_ < static_cast<int>(levels.size()))
        {
            const std::string path =
                PathToUtf8(
                    levels[selectedLevelIndex_].directory);

            ImGui::TextWrapped(
                "%s", path.c_str());
        }

        ImGui::Separator();

        const bool canOpen =
            selectedLevelIndex_ >= 0 &&
            selectedLevelIndex_ < static_cast<int>(levels.size());

        ImGui::BeginDisabled(!canOpen);

        if (ImGui::Button("Open", ImVec2(120, 0)) && canOpen)
        {
            requestedLevel_ =
                levels[selectedLevelIndex_].directory;

            openLevelRequested_ = true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Refresh", ImVec2(120, 0)))
            refreshLevelsRequested_ = true;

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    void EditorUI::AddConsoleMessage(
        std::string message)
    {
        consoleLines_.push_back(
            std::move(message));

        if (consoleLines_.size() > 500)
            consoleLines_.erase(consoleLines_.begin());

        scrollConsole_ = true;
    }

    bool EditorUI::ConsumeNewSceneRequest() noexcept
    {
        return std::exchange(newSceneRequested_, false);
    }

    bool EditorUI::ConsumeOpenLevelRequest(
        std::filesystem::path& directory)
    {
        if (!std::exchange(openLevelRequested_, false))
            return false;

        directory = std::move(requestedLevel_);
        requestedLevel_.clear();

        return true;
    }

    bool EditorUI::ConsumeRefreshLevelsRequest() noexcept
    {
        return std::exchange(refreshLevelsRequested_, false);
    }

    bool EditorUI::ConsumeExitRequest() noexcept
    {
        return std::exchange(exitRequested_, false);
    }

    bool EditorUI::ViewportHovered() const noexcept
    {
        return viewportHovered_;
    }

    float EditorUI::ViewportWheel() const noexcept
    {
        return viewportWheel_;
    }

    const RECT& EditorUI::ViewportRectangle() const noexcept
    {
        return viewportRectangle_;
    }
}