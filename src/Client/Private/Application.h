#pragma once

#include "Graphics/Renderer.h"
#include "Platform/Window.h"

#include "Core/Runtime.h"

namespace client
{
    class Application final
    {
    public:
        int Run();

    private:
        [[nodiscard]]
        bool Initialize();

        void Shutdown();

        core::Runtime runtime_;

        platform::Window window_;
        graphics::Renderer renderer_;
    };
}