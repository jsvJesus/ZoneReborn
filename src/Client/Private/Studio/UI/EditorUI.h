#pragma once

#include <Windows.h>
#include <d3d11.h>

namespace studio
{
    class EditorScene;
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
            ID3D11ShaderResourceView* sceneTexture);

        void Render();

        [[nodiscard]]
        bool ConsumeNewSceneRequest() noexcept;

        [[nodiscard]]
        bool ConsumeExitRequest() noexcept;

    private:
        void BuildMainMenu();
        void BuildDockSpace();

        void BuildViewport(
            ID3D11ShaderResourceView* sceneTexture);

        void BuildSceneOutliner(
            const EditorScene& scene);

        void BuildDetails();
        void BuildContentBrowser();
        void BuildConsole();

        void BuildDefaultLayout(
            unsigned int dockspaceId);

        bool initialized_ = false;
        bool layoutInitialized_ = false;

        bool newSceneRequested_ = false;
        bool exitRequested_ = false;

        bool showViewport_ = true;
        bool showOutliner_ = true;
        bool showDetails_ = true;
        bool showContentBrowser_ = true;
        bool showConsole_ = true;
    };
}