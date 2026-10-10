#pragma once

#include <Windows.h>
#include <d3d11.h>

#include <filesystem>
#include <string>
#include <vector>

namespace studio
{
    class EditorScene;
    class LevelCatalog;
}

namespace studio::ui
{
    class EditorUI final
    {
    public:
        EditorUI() = default;
        ~EditorUI();

        EditorUI(const EditorUI&) = delete;
        EditorUI& operator=(const EditorUI&) = delete;

        [[nodiscard]]
        bool Initialize(
            HWND window,
            ID3D11Device* device,
            ID3D11DeviceContext* context);

        void Shutdown();

        void BeginFrame(
            const EditorScene& scene,
            const LevelCatalog& catalog,
            ID3D11ShaderResourceView* sceneTexture);

        void Render();

        void AddConsoleMessage(std::string message);

        [[nodiscard]]
        bool ConsumeNewSceneRequest() noexcept;

        [[nodiscard]]
        bool ConsumeOpenLevelRequest(
            std::filesystem::path& directory);

        [[nodiscard]]
        bool ConsumeRefreshLevelsRequest() noexcept;

        [[nodiscard]]
        bool ConsumeExitRequest() noexcept;

        [[nodiscard]]
        bool ViewportHovered() const noexcept;

        [[nodiscard]]
        float ViewportWheel() const noexcept;

        [[nodiscard]]
        const RECT& ViewportRectangle() const noexcept;

    private:
        void BuildMainMenu();
        void BuildDockSpace();
        void BuildDefaultLayout(unsigned int dockspaceId);

        void BuildViewport(
            ID3D11ShaderResourceView* sceneTexture);

        void BuildSceneOutliner(
            const EditorScene& scene);

        void BuildDetails(
            const EditorScene& scene);

        void BuildContentBrowser(
            const LevelCatalog& catalog);

        void BuildConsole();

        void BuildOpenLevelDialog(
            const LevelCatalog& catalog);

        HWND window_ = nullptr;

        bool initialized_ = false;
        bool layoutInitialized_ = false;

        bool newSceneRequested_ = false;
        bool openDialogRequested_ = false;
        bool openLevelRequested_ = false;
        bool refreshLevelsRequested_ = false;
        bool exitRequested_ = false;

        bool viewportHovered_ = false;

        float viewportWheel_ = 0.0f;

        RECT viewportRectangle_{};

        int selectedLevelIndex_ = -1;

        std::filesystem::path requestedLevel_;

        std::vector<std::string> consoleLines_;
        bool scrollConsole_ = false;

        bool showViewport_ = true;
        bool showOutliner_ = true;
        bool showDetails_ = true;
        bool showContentBrowser_ = true;
        bool showConsole_ = true;

        void BuildToolbar();

        float uiScale_ = 1.0f;

        char levelSearch_[128]{};
        char contentSearch_[128]{};

        int contentSelectedIndex_ = -1;
    };
}