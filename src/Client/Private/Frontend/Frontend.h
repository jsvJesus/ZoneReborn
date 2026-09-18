#pragma once

#include "Core/Resources/ResourceFileSystem.h"

#include <Windows.h>
#include <WebView2.h>
#include <wrl/client.h>

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace client::frontend
{
    enum class FrontendEventType
    {
        Login,
        Exit,
        OpenUrl,
        Play
    };

    struct FrontendEvent final
    {
        FrontendEventType type =
            FrontendEventType::Exit;

        std::string login;
        std::string password;
        std::string serverId;
        std::string url;

        bool rememberLogin =
            false;
    };

    class OriginalFrontend final
    {
    public:
        OriginalFrontend() = default;
        ~OriginalFrontend();

        OriginalFrontend(
            const OriginalFrontend&) =
                delete;

        OriginalFrontend& operator=(
            const OriginalFrontend&) =
                delete;

        [[nodiscard]]
        bool Initialize(
            HWND parentWindow,
            const std::filesystem::path& gameRoot,
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        void Shutdown();

        void Resize();

        [[nodiscard]]
        bool ConsumeEvent(
            FrontendEvent& event);

        [[nodiscard]]
        bool ConsumeFatalError(
            std::string& error);

        void SendLoginError(
            const std::string& message);

        void SendLoginAccepted();

        void Hide();

        void Show();

    private:
        void HandleWebMessage(
            ICoreWebView2WebMessageReceivedEventArgs* args);

        void SendLocalization(
            const std::string& language,
            const std::vector<std::string>& paths);

        void ExecuteScriptUtf8(
            const std::string& script);

        void SetFatalError(
            std::string error);

        HWND parentWindow_ =
            nullptr;

        std::filesystem::path
            gameRoot_;

        const core::resources::ResourceFileSystem*
            resources_ =
                nullptr;

        Microsoft::WRL::ComPtr<
            ICoreWebView2Environment>
                environment_;

        Microsoft::WRL::ComPtr<
            ICoreWebView2Controller>
                controller_;

        Microsoft::WRL::ComPtr<
            ICoreWebView2>
                webView_;

        EventRegistrationToken
            webMessageToken_{};

        std::deque<FrontendEvent>
            events_;

        std::string
            fatalError_;

        bool webMessageRegistered_ =
            false;

        bool comInitialized_ =
            false;

        bool shuttingDown_ =
            false;
    };
}