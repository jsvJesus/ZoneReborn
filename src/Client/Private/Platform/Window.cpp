#include "Platform/Window.h"

namespace
{
    constexpr wchar_t WindowClassName[] =
        L"ClientRenderWindow";
}

namespace client::platform
{
    Window::~Window()
    {
        Shutdown();
    }

    bool Window::Initialize(
        const std::uint32_t width,
        const std::uint32_t height,
        const wchar_t* title,
        std::string& error)
    {
        Shutdown();

        error.clear();

        instance_ =
            GetModuleHandleW(nullptr);

        if (instance_ == nullptr)
        {
            error =
                "Unable to obtain application instance.";

            return false;
        }

        WNDCLASSEXW windowClass{};

        windowClass.cbSize =
            sizeof(windowClass);

        windowClass.style =
            CS_HREDRAW |
            CS_VREDRAW |
            CS_OWNDC;

        windowClass.lpfnWndProc =
            WindowProcedure;

        windowClass.hInstance =
            instance_;

        windowClass.hCursor =
            LoadCursorW(
                nullptr,
                IDC_ARROW);

        windowClass.hbrBackground =
            nullptr;

        windowClass.lpszClassName =
            WindowClassName;

        if (RegisterClassExW(
                &windowClass) == 0)
        {
            error =
                "Unable to register render window class.";

            instance_ = nullptr;

            return false;
        }

        classRegistered_ = true;

        const DWORD style =
            WS_OVERLAPPED |
            WS_CAPTION |
            WS_SYSMENU |
            WS_MINIMIZEBOX;

        RECT rectangle
        {
            0,
            0,
            static_cast<LONG>(width),
            static_cast<LONG>(height)
        };

        if (!AdjustWindowRect(
                &rectangle,
                style,
                FALSE))
        {
            error =
                "Unable to calculate render window size.";

            Shutdown();

            return false;
        }

        const int windowWidth =
            rectangle.right -
            rectangle.left;

        const int windowHeight =
            rectangle.bottom -
            rectangle.top;

        window_ =
            CreateWindowExW(
                0,
                WindowClassName,
                title,
                style,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                windowWidth,
                windowHeight,
                nullptr,
                nullptr,
                instance_,
                nullptr);

        if (window_ == nullptr)
        {
            error =
                "Unable to create render window.";

            Shutdown();

            return false;
        }

        width_ = width;
        height_ = height;

        ShowWindow(
            window_,
            SW_SHOW);

        UpdateWindow(
            window_);

        return true;
    }

    void Window::Shutdown()
    {
        if (window_ != nullptr)
        {
            DestroyWindow(
                window_);

            window_ = nullptr;
        }

        if (classRegistered_ &&
            instance_ != nullptr)
        {
            UnregisterClassW(
                WindowClassName,
                instance_);

            classRegistered_ = false;
        }

        instance_ = nullptr;

        width_ = 0;
        height_ = 0;
    }

    bool Window::ProcessMessages()
    {
        MSG message{};

        while (PeekMessageW(
            &message,
            nullptr,
            0,
            0,
            PM_REMOVE))
        {
            if (message.message ==
                WM_QUIT)
            {
                return false;
            }

            TranslateMessage(
                &message);

            DispatchMessageW(
                &message);
        }

        return true;
    }

    HWND Window::NativeHandle() const noexcept
    {
        return window_;
    }

    std::uint32_t Window::Width() const noexcept
    {
        return width_;
    }

    std::uint32_t Window::Height() const noexcept
    {
        return height_;
    }

    LRESULT CALLBACK Window::WindowProcedure(
        const HWND window,
        const UINT message,
        const WPARAM wParam,
        const LPARAM lParam)
    {
        switch (message)
        {
            case WM_CLOSE:
            {
                DestroyWindow(
                    window);

                return 0;
            }

            case WM_DESTROY:
            {
                PostQuitMessage(0);

                return 0;
            }

            default:
            {
                break;
            }
        }

        return DefWindowProcW(
            window,
            message,
            wParam,
            lParam);
    }
}