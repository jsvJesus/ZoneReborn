#pragma once

#include "Core/Runtime.h"

#include "Graphics/Renderer.h"
#include "Graphics/SceneRenderData.h"

#include "Input/CameraController.h"
#include "Platform/Window.h"

#include <chrono>
#include <string>

namespace studio
{
    class Application final
    {
    public:
        explicit Application(
            std::string spaceName);

        int Run();

    private:
        [[nodiscard]]
        bool Initialize();

        [[nodiscard]]
        bool Update();

        void Shutdown();

        std::string
            spaceName_;

        core::Runtime
            runtime_;

        client::platform::Window
            window_;

        client::graphics::Renderer
            renderer_;

        client::graphics::SceneRenderData
            scene_;

        client::input::CameraController
            camera_;

        std::chrono::steady_clock::time_point
            previousFrame_{};

        bool rendererInitialized_ =
            false;

        bool windowInitialized_ =
            false;

        bool runtimeInitialized_ =
            false;
    };
}