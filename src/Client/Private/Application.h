#pragma once

#include "Graphics/Renderer.h"
#include "Input/CameraController.h"
#include "Platform/Window.h"

#include "Core/Runtime.h"

#include <string>

namespace client
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

        void Shutdown();

        std::string
            spaceName_;

        core::Runtime
            runtime_;

        platform::Window
            window_;

        graphics::Renderer
            renderer_;

        input::CameraController
            cameraController_;
    };
}