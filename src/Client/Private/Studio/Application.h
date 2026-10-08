#pragma once

#include "Core/Runtime.h"

#include "Graphics/Renderer.h"
#include "Input/CameraController.h"
#include "Platform/Window.h"

#include "Studio/EditorScene.h"
#include "Studio/UI/EditorUI.h"

#include <string>

namespace studio
{
    class Application final
    {
    public:
        Application() = default;

        int Run();

    private:
        [[nodiscard]]
        bool Initialize();

        [[nodiscard]]
        bool Update();

        [[nodiscard]]
        bool NewScene();

        static void RenderOverlay(
            void* userData);

        void Shutdown();

        core::Runtime runtime_;

        client::platform::Window window_;

        client::graphics::Renderer renderer_;

        client::input::CameraController camera_;

        EditorScene scene_;

        ui::EditorUI editorUI_;

        bool rendererInitialized_ = false;
        bool uiInitialized_ = false;
        bool windowInitialized_ = false;
        bool runtimeInitialized_ = false;
    };
}