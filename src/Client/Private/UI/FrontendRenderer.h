#pragma once

#include "UI/FrontendView.h"

#include "Core/Resources/ResourceFileSystem.h"

#include <Windows.h>

#include <cstdint>
#include <memory>
#include <string>

namespace client::ui
{
    class FrontendRenderer final
    {
    public:
        FrontendRenderer();
        ~FrontendRenderer();

        FrontendRenderer(
            const FrontendRenderer&) =
                delete;

        FrontendRenderer& operator=(
            const FrontendRenderer&) =
                delete;

        [[nodiscard]]
        bool Initialize(
            HWND window,
            std::uint32_t width,
            std::uint32_t height,
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        [[nodiscard]]
        bool RenderStartupSplash(
            std::string& error);

        [[nodiscard]]
        bool RenderLogin(
            const LoginView& view,
            std::string& error);

        [[nodiscard]]
        bool RenderBackground(
            std::string& error);

        void Shutdown();

    private:
        struct State;

        std::unique_ptr<State>
            state_;
    };
}