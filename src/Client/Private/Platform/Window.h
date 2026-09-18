#pragma once

#include <Windows.h>

#include <array>
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

        [[nodiscard]]
        std::wstring ConsumeTextInput();

        [[nodiscard]]
        bool ConsumeKeyPress(
            UINT virtualKey) noexcept;

        [[nodiscard]]
        bool ConsumeLeftMousePress(
            POINT& position) noexcept;

        [[nodiscard]]
        POINT MousePosition() const noexcept;

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

        POINT mousePosition_{};

        bool leftMousePressed_ =
            false;

        std::array<bool, 256>
            keyPressed_{};

        std::wstring
            textInput_;

        bool classRegistered_ =
            false;
    };
}