#include "Frontend/LoginScreen.h"

#include "Frontend/LoginLayout.h"

#include <Windows.h>

#include <algorithm>
#include <utility>

namespace
{
    constexpr std::size_t
        MaximumLoginLength =
            64;

    constexpr std::size_t
        MaximumPasswordLength =
            64;
}

namespace client::frontend
{
    void LoginScreen::Initialize(
        const std::string& rememberedLogin)
    {
        login_.clear();
        password_.clear();
        message_.clear();

        focus_ =
            ui::LoginFocus::Login;

        rememberLogin_ =
            false;

        authenticating_ =
            false;

        submitRequested_ =
            false;

        if (!rememberedLogin.empty())
        {
            const int requiredLength =
                MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    rememberedLogin.data(),
                    static_cast<int>(
                        rememberedLogin.size()),
                    nullptr,
                    0);

            if (requiredLength >
                0)
            {
                login_.resize(
                    static_cast<
                        std::size_t>(
                            requiredLength));

                MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    rememberedLogin.data(),
                    static_cast<int>(
                        rememberedLogin.size()),
                    login_.data(),
                    requiredLength);

                rememberLogin_ =
                    true;

                focus_ =
                    ui::LoginFocus::Password;
            }
        }
    }

    void LoginScreen::Update(
        platform::Window& window)
    {
        if (authenticating_)
        {
            window.ConsumeTextInput();

            return;
        }

        POINT clickPosition{};

        if (window.ConsumeLeftMousePress(
                clickPosition))
        {
            const float mouseX =
                static_cast<float>(
                    clickPosition.x);

            const float mouseY =
                static_cast<float>(
                    clickPosition.y);

            if (layout::LoginEdit.Contains(
                    mouseX,
                    mouseY))
            {
                focus_ =
                    ui::LoginFocus::Login;
            }
            else if (
                layout::PasswordEdit.Contains(
                    mouseX,
                    mouseY))
            {
                focus_ =
                    ui::LoginFocus::Password;
            }
            else if (
                layout::RememberLogin.Contains(
                    mouseX,
                    mouseY))
            {
                rememberLogin_ =
                    !rememberLogin_;
            }
            else if (
                layout::LoginButton.Contains(
                    mouseX,
                    mouseY))
            {
                Submit();
            }
            else if (
                layout::CreateAccount.Contains(
                    mouseX,
                    mouseY))
            {
                message_ =
                    L"Веб-регистрация пока недоступна. "
                    L"Тестовый аккаунт: test / test123";
            }
            else if (
                layout::RestoreAccount.Contains(
                    mouseX,
                    mouseY))
            {
                message_ =
                    L"Восстановление аккаунта "
                    L"будет подключено позже.";
            }
            else
            {
                focus_ =
                    ui::LoginFocus::None;
            }
        }

        if (window.ConsumeKeyPress(
                VK_TAB))
        {
            if (focus_ ==
                ui::LoginFocus::Login)
            {
                focus_ =
                    ui::LoginFocus::Password;
            }
            else
            {
                focus_ =
                    ui::LoginFocus::Login;
            }
        }

        if (window.ConsumeKeyPress(
                VK_RETURN))
        {
            Submit();
        }

        if (window.ConsumeKeyPress(
                VK_BACK))
        {
            if (focus_ ==
                    ui::LoginFocus::Login &&
                !login_.empty())
            {
                login_.pop_back();
            }
            else if (
                focus_ ==
                    ui::LoginFocus::Password &&
                !password_.empty())
            {
                password_.pop_back();
            }
        }

        std::wstring text =
            window.ConsumeTextInput();

        if (text.empty())
        {
            return;
        }

        if (focus_ ==
            ui::LoginFocus::Login)
        {
            for (const wchar_t character :
                 text)
            {
                if (login_.size() >=
                    MaximumLoginLength)
                {
                    break;
                }

                if (character ==
                        L'\r' ||
                    character ==
                        L'\n' ||
                    character ==
                        L'\t' ||
                    character ==
                        L' ')
                {
                    continue;
                }

                login_.push_back(
                    character);
            }
        }
        else if (
            focus_ ==
            ui::LoginFocus::Password)
        {
            for (const wchar_t character :
                 text)
            {
                if (password_.size() >=
                    MaximumPasswordLength)
                {
                    break;
                }

                if (character ==
                        L'\r' ||
                    character ==
                        L'\n' ||
                    character ==
                        L'\t')
                {
                    continue;
                }

                password_.push_back(
                    character);
            }
        }
    }

    bool LoginScreen::ConsumeLoginRequest(
        LoginRequest& request)
    {
        if (!submitRequested_)
        {
            return false;
        }

        submitRequested_ =
            false;

        request.login =
            ToUtf8(
                login_);

        request.password =
            ToUtf8(
                password_);

        request.rememberLogin =
            rememberLogin_;

        return true;
    }

    void LoginScreen::SetMessage(
        std::wstring message)
    {
        message_ =
            std::move(
                message);
    }

    void LoginScreen::SetAuthenticating(
        const bool value) noexcept
    {
        authenticating_ =
            value;
    }

    ui::LoginView LoginScreen::View() const
    {
        ui::LoginView result;

        result.login =
            login_;

        result.password =
            password_;

        result.message =
            message_;

        result.focus =
            focus_;

        result.rememberLogin =
            rememberLogin_;

        result.authenticating =
            authenticating_;

        return result;
    }

    std::string LoginScreen::ToUtf8(
        const std::wstring& value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredLength =
            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(
                    value.size()),
                nullptr,
                0,
                nullptr,
                nullptr);

        if (requiredLength <=
            0)
        {
            return {};
        }

        std::string result;

        result.resize(
            static_cast<std::size_t>(
                requiredLength));

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(
                value.size()),
            result.data(),
            requiredLength,
            nullptr,
            nullptr);

        return result;
    }

    void LoginScreen::Submit()
    {
        message_.clear();

        if (login_.empty())
        {
            message_ =
                L"Введите логин.";

            focus_ =
                ui::LoginFocus::Login;

            return;
        }

        if (password_.empty())
        {
            message_ =
                L"Введите пароль.";

            focus_ =
                ui::LoginFocus::Password;

            return;
        }

        submitRequested_ =
            true;
    }
}