#pragma once

#include "Platform/Window.h"
#include "UI/FrontendView.h"

#include <string>

namespace client::frontend
{
    struct LoginRequest final
    {
        std::string login;
        std::string password;

        bool rememberLogin =
            false;
    };

    class LoginScreen final
    {
    public:
        void Initialize(
            const std::string& rememberedLogin);

        void Update(
            platform::Window& window);

        [[nodiscard]]
        bool ConsumeLoginRequest(
            LoginRequest& request);

        void SetMessage(
            std::wstring message);

        void SetAuthenticating(
            bool value) noexcept;

        [[nodiscard]]
        ui::LoginView View() const;

    private:
        static std::string ToUtf8(
            const std::wstring& value);

        void Submit();

        std::wstring login_;
        std::wstring password_;
        std::wstring message_;

        ui::LoginFocus focus_ =
            ui::LoginFocus::Login;

        bool rememberLogin_ =
            false;

        bool authenticating_ =
            false;

        bool submitRequested_ =
            false;
    };
}