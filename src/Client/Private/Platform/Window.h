#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>

namespace client::platform
{
    class Window final
    {
    public:
        Window() = default;
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        [[nodiscard]]
        bool Initialize(
            std::uint32_t width,
            std::uint32_t height,
            const wchar_t* title,
            std::string& error);

        void Shutdown();

        [[nodiscard]]
        bool ProcessMessages();

        [[nodiscard]]
        HWND NativeHandle() const noexcept;

        [[nodiscard]]
        std::uint32_t Width() const noexcept;

        [[nodiscard]]
        std::uint32_t Height() const noexcept;

        [[nodiscard]]
        float ConsumeMouseWheelDelta() noexcept;

    private:
        static LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);

        HINSTANCE instance_ =
            nullptr;

        HWND window_ =
            nullptr;

        std::uint32_t width_ =
            0;

        std::uint32_t height_ =
            0;

        int mouseWheelDelta_ =
            0;

        bool classRegistered_ =
            false;
    };
}