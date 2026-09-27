#pragma once

#include "Graphics/Renderer.h"
#include "Input/CameraController.h"
#include "Platform/Window.h"

#include "Core/Runtime.h"

#include <chrono>
#include <string>
#include <string_view>

namespace client::world
{
    class Session final
    {
    public:
        [[nodiscard]]
        bool Load(
            core::Runtime& runtime,
            platform::Window& window,
            graphics::Renderer& renderer,
            std::string_view spaceName,
            std::string& error);

        void Update(
            platform::Window& window,
            graphics::Renderer& renderer) noexcept;

        void Unload(
            graphics::Renderer& renderer) noexcept;

        [[nodiscard]]
        bool IsLoaded() const noexcept;

        [[nodiscard]]
        std::string_view SpaceName() const noexcept;

    private:
        input::CameraController
            cameraController_;

        std::chrono::steady_clock::time_point
            previousUpdateTime_ =
                std::chrono::steady_clock::now();

        std::string
            spaceName_;

        bool loaded_ =
            false;
    };
}