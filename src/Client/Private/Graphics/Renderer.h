#pragma once

#include "Graphics/SceneRenderData.h"

#include <Windows.h>

#include <cstdint>
#include <memory>
#include <string>

namespace client::graphics
{
    class Renderer final
    {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            HWND window,
            std::uint32_t width,
            std::uint32_t height,
            std::string& error);

        [[nodiscard]]
        bool SetScene(
            const SceneRenderData& scene,
            std::string& error);

        [[nodiscard]]
        bool Render(
            std::string& error);

        void Shutdown();

    private:
        struct State;

        std::unique_ptr<State> state_;
    };
}