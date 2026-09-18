#pragma once

#include "Platform/Window.h"
#include "UI/FrontendView.h"

#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <vector>

namespace client::frontend
{
    struct LoginRequest final
    {
        std::string login;
        std::string password;
        std::string server;

        bool rememberLogin =
            false;
    };

    class LoginScreen final
    {
    public:
        void Initialize(
            const core::resources::ResourceFileSystem& resources,
            const std::string& rememberedLogin);

        void Update(
            platform::Window& window);

        [[nodiscard]]
        bool ConsumeLoginRequest(
            LoginRequest& request);

        [[nodiscard]]
        bool ConsumeExitRequest() noexcept;

        void SetMessage(
            std::wstring message);

        void SetAuthenticating(
            bool value) noexcept;

        [[nodiscard]]
        ui::LoginView View() const;

    private:
        void LoadServers(
            const core::resources::ResourceFileSystem& resources);

        void LoadVersion(
            const core::resources::ResourceFileSystem& resources);

        void Submit();

        void SelectLanguage(
            ui::FrontendLanguage language);

        [[nodiscard]]
        std::wstring Localized(
            const wchar_t* russian,
            const wchar_t* english) const;

        [[nodiscard]]
        static std::string ToUtf8(
            const std::wstring& value);

        [[nodiscard]]
        static std::wstring FromUtf8(
            const std::string& value);

        std::wstring login_;
        std::wstring password_;

        std::wstring message_;

        std::vector<std::wstring>
            servers_;

        std::wstring version_;

        std::size_t selectedServer_ =
            0;

        ui::LoginFocus focus_ =
            ui::LoginFocus::Login;

        ui::FrontendLanguage language_ =
            ui::FrontendLanguage::Russian;

        bool rememberLogin_ =
            false;

        bool authenticating_ =
            false;

        bool submitRequested_ =
            false;

        bool exitRequested_ =
            false;

        bool serverListOpen_ =
            false;
    };
}