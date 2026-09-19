#include "Platform/Window.h"
#include "Resources/Resource.h"

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
            GetModuleHandleW(
                nullptr);

        if (instance_ ==
            nullptr)
        {
            error =
                "Unable to obtain application instance.";

            return false;
        }

        WNDCLASSEXW windowClass{};

        windowClass.cbSize =
            sizeof(
                windowClass);

        windowClass.style =
            CS_HREDRAW |
            CS_VREDRAW |
            CS_OWNDC;

        windowClass.lpfnWndProc =
            WindowProcedure;

        windowClass.hInstance =
            instance_;

        windowClass.hIcon =
            static_cast<HICON>(
                LoadImageW(
                    instance_,
                    MAKEINTRESOURCEW(
                        IDI_CLIENT_ICON),
                    IMAGE_ICON,
                    GetSystemMetrics(
                        SM_CXICON),
                    GetSystemMetrics(
                        SM_CYICON),
                    LR_DEFAULTCOLOR |
                    LR_SHARED));

        windowClass.hIconSm =
            static_cast<HICON>(
                LoadImageW(
                    instance_,
                    MAKEINTRESOURCEW(
                        IDI_CLIENT_ICON),
                    IMAGE_ICON,
                    GetSystemMetrics(
                        SM_CXSMICON),
                    GetSystemMetrics(
                        SM_CYSMICON),
                    LR_DEFAULTCOLOR |
                    LR_SHARED));

        windowClass.hCursor =
            LoadCursorW(
                nullptr,
                IDC_ARROW);

        windowClass.hbrBackground =
            reinterpret_cast<HBRUSH>(
                GetStockObject(BLACK_BRUSH));

        windowClass.lpszClassName =
            WindowClassName;

        if (RegisterClassExW(
                &windowClass) ==
            0)
        {
            error =
                "Unable to register window class.";

            instance_ =
                nullptr;

            return false;
        }

        classRegistered_ =
            true;

        const POINT monitorPoint
{
    0,
    0
};

        const HMONITOR monitor =
            MonitorFromPoint(
                monitorPoint,
                MONITOR_DEFAULTTOPRIMARY);

        if (monitor ==
            nullptr)
        {
            error =
                "Unable to obtain primary monitor.";

            Shutdown();

            return false;
        }

        MONITORINFO monitorInfo{};

        monitorInfo.cbSize =
            sizeof(
                monitorInfo);

        if (!GetMonitorInfoW(
                monitor,
                &monitorInfo))
        {
            error =
                "Unable to obtain primary monitor information.";

            Shutdown();

            return false;
        }

        const RECT& monitorRectangle =
            monitorInfo.rcMonitor;

        const int windowWidth =
            monitorRectangle.right -
            monitorRectangle.left;

        const int windowHeight =
            monitorRectangle.bottom -
            monitorRectangle.top;

        if (windowWidth <= 0 ||
            windowHeight <= 0)
        {
            error =
                "Primary monitor has invalid dimensions.";

            Shutdown();

            return false;
        }

        constexpr DWORD style = WS_OVERLAPPEDWINDOW;

        window_ =
            CreateWindowExW(
                WS_EX_APPWINDOW,
                WindowClassName,
                title,
                style,
                monitorRectangle.left,
                monitorRectangle.top,
                windowWidth,
                windowHeight,
                nullptr,
                nullptr,
                instance_,
                this);

        if (window_ ==
            nullptr)
        {
            error =
                "Unable to create client window.";

            Shutdown();

            return false;
        }

        width_ =
            static_cast<std::uint32_t>(
                windowWidth);

        height_ =
            static_cast<std::uint32_t>(
                windowHeight);

        mouseWheelDelta_ =
            0;

        mousePosition_ =
            {};

        leftMousePressed_ =
            false;

        keyPressed_.fill(
            false);

        textInput_.clear();

        ShowWindow(window_, SW_MAXIMIZE);

        UpdateWindow(
            window_);

        return true;
    }

    void Window::Shutdown()
    {
        if (window_ !=
            nullptr)
        {
            DestroyWindow(
                window_);

            window_ =
                nullptr;
        }

        if (classRegistered_ &&
            instance_ !=
                nullptr)
        {
            UnregisterClassW(
                WindowClassName,
                instance_);

            classRegistered_ =
                false;
        }

        instance_ =
            nullptr;

        width_ =
            0;

        height_ =
            0;

        mouseWheelDelta_ =
            0;

        mousePosition_ =
            {};

        leftMousePressed_ =
            false;

        keyPressed_.fill(
            false);

        textInput_.clear();
    }

    bool Window::ProcessMessages()
    {
        leftMousePressed_ =
            false;

        keyPressed_.fill(
            false);

        textInput_.clear();

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

    float Window::ConsumeMouseWheelDelta() noexcept
    {
        const int delta =
            mouseWheelDelta_;

        mouseWheelDelta_ =
            0;

        return
            static_cast<float>(
                delta) /
            static_cast<float>(
                WHEEL_DELTA);
    }

    std::wstring Window::ConsumeTextInput()
    {
        std::wstring result =
            std::move(
                textInput_);

        textInput_.clear();

        return result;
    }

    bool Window::ConsumeKeyPress(
        const UINT virtualKey) noexcept
    {
        if (virtualKey >=
            keyPressed_.size())
        {
            return false;
        }

        const bool pressed =
            keyPressed_[
                virtualKey];

        keyPressed_[
            virtualKey] =
                false;

        return pressed;
    }

    bool Window::ConsumeLeftMousePress(
        POINT& position) noexcept
    {
        if (!leftMousePressed_)
        {
            return false;
        }

        position =
            mousePosition_;

        leftMousePressed_ =
            false;

        return true;
    }

    POINT Window::MousePosition() const noexcept
    {
        return mousePosition_;
    }

    LRESULT CALLBACK Window::WindowProcedure(
        const HWND window,
        const UINT message,
        const WPARAM wParam,
        const LPARAM lParam)
    {
        Window* instance =
            reinterpret_cast<Window*>(
                GetWindowLongPtrW(
                    window,
                    GWLP_USERDATA));

        if (message ==
            WM_NCCREATE)
        {
            const auto* creation =
                reinterpret_cast<
                    CREATESTRUCTW*>(
                        lParam);

            instance =
                static_cast<Window*>(
                    creation->lpCreateParams);

            SetWindowLongPtrW(
                window,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(
                    instance));
        }

        if (instance !=
            nullptr)
        {
            switch (message)
            {
                case WM_MOUSEMOVE:
                {
                    instance->mousePosition_.x =
                        static_cast<short>(
                            LOWORD(
                                lParam));

                    instance->mousePosition_.y =
                        static_cast<short>(
                            HIWORD(
                                lParam));

                    break;
                }

                case WM_LBUTTONDOWN:
                {
                    instance->mousePosition_.x =
                        static_cast<short>(
                            LOWORD(
                                lParam));

                    instance->mousePosition_.y =
                        static_cast<short>(
                            HIWORD(
                                lParam));

                    instance->leftMousePressed_ =
                        true;

                    SetFocus(
                        window);

                    return 0;
                }

                case WM_MOUSEWHEEL:
                {
                    const auto wheelDelta =
                        static_cast<short>(
                            HIWORD(
                                wParam));

                    instance->mouseWheelDelta_ +=
                        wheelDelta;

                    return 0;
                }

                case WM_KEYDOWN:
                {
                    if (wParam <
                        instance->
                            keyPressed_.
                            size())
                    {
                        instance->
                            keyPressed_[
                                static_cast<
                                    std::size_t>(
                                        wParam)] =
                            true;
                    }

                    break;
                }

                case WM_CHAR:
                {
                    const wchar_t character =
                        static_cast<wchar_t>(
                            wParam);

                    if (character >=
                        32)
                    {
                        instance->
                            textInput_.
                            push_back(
                                character);
                    }

                    return 0;
                }

                default:
                {
                    break;
                }
            }
        }

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
                PostQuitMessage(
                    0);

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