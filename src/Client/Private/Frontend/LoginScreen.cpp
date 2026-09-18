#include "Frontend/LoginScreen.h"

#include "Frontend/LoginLayout.h"

#include <Windows.h>

#include <cstddef>
#include <utility>

namespace
{
    constexpr std::size_t
        MaximumLoginLength =
            64;

    constexpr std::size_t
        MaximumPasswordLength =
            64;

    constexpr char
        ServersResource[] =
            "res/scripts/client/data/servers_config.json";

    constexpr char
        VersionResource[] =
            "res/scripts/common/VersionSO.pyc";
}

namespace client::frontend
{
    void LoginScreen::Initialize(
        const core::resources::ResourceFileSystem& resources,
        const std::string& rememberedLogin)
    {
        login_.clear();
        password_.clear();
        message_.clear();

        servers_.clear();
        version_.clear();

        selectedServer_ =
            0;

        focus_ =
            ui::LoginFocus::Login;

        language_ =
            ui::FrontendLanguage::Russian;

        rememberLogin_ =
            false;

        authenticating_ =
            false;

        submitRequested_ =
            false;

        exitRequested_ =
            false;

        serverListOpen_ =
            false;

        LoadServers(
            resources);

        LoadVersion(
            resources);

        if (!rememberedLogin.empty())
        {
            login_ =
                FromUtf8(
                    rememberedLogin);

            rememberLogin_ =
                true;

            focus_ =
                ui::LoginFocus::Password;
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

        POINT click{};

        if (window.ConsumeLeftMousePress(
                click))
        {
            const float mouseX =
                static_cast<float>(
                    click.x) *
                layout::ReferenceWidth /
                static_cast<float>(
                    window.Width());

            const float mouseY =
                static_cast<float>(
                    click.y) *
                layout::ReferenceHeight /
                static_cast<float>(
                    window.Height());

            if (serverListOpen_)
            {
                bool serverSelected =
                    false;

                for (std::size_t index = 0;
                     index < servers_.size();
                     ++index)
                {
                    if (layout::ServerListItem(
                            index).
                        Contains(
                            mouseX,
                            mouseY))
                    {
                        selectedServer_ =
                            index;

                        serverListOpen_ =
                            false;

                        serverSelected =
                            true;

                        break;
                    }
                }

                if (serverSelected)
                {
                    return;
                }
            }

            if (layout::LoginEdit.Contains(
                    mouseX,
                    mouseY))
            {
                focus_ =
                    ui::LoginFocus::Login;

                serverListOpen_ =
                    false;
            }
            else if (
                layout::PasswordEdit.Contains(
                    mouseX,
                    mouseY))
            {
                focus_ =
                    ui::LoginFocus::Password;

                serverListOpen_ =
                    false;
            }
            else if (
                layout::RememberToggle.Contains(
                    mouseX,
                    mouseY))
            {
                rememberLogin_ =
                    !rememberLogin_;

                serverListOpen_ =
                    false;
            }
            else if (
                layout::ServerValue.Contains(
                    mouseX,
                    mouseY) ||
                layout::ServerButton.Contains(
                    mouseX,
                    mouseY))
            {
                serverListOpen_ =
                    !serverListOpen_;
            }
            else if (
                layout::LoginButton.Contains(
                    mouseX,
                    mouseY))
            {
                serverListOpen_ =
                    false;

                Submit();
            }
            else if (
                layout::RegisterLink.Contains(
                    mouseX,
                    mouseY))
            {
                serverListOpen_ =
                    false;

                message_ =
                    Localized(
                        L"Регистрация через сайт будет подключена позже.",
                        L"Web registration will be connected later.");
            }
            else if (
                layout::EnglishButton.Contains(
                    mouseX,
                    mouseY))
            {
                SelectLanguage(
                    ui::FrontendLanguage::English);
            }
            else if (
                layout::RussianButton.Contains(
                    mouseX,
                    mouseY))
            {
                SelectLanguage(
                    ui::FrontendLanguage::Russian);
            }
            else if (
                layout::ExitButton.Contains(
                    mouseX,
                    mouseY))
            {
                exitRequested_ =
                    true;
            }
            else if (
                layout::SettingsButton.Contains(
                    mouseX,
                    mouseY))
            {
                serverListOpen_ =
                    false;

                message_ =
                    Localized(
                        L"Меню настроек будет восстановлено следующим экраном.",
                        L"Settings menu will be restored next.");
            }
            else
            {
                serverListOpen_ =
                    false;

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
                VK_ESCAPE))
        {
            if (serverListOpen_)
            {
                serverListOpen_ =
                    false;
            }
            else
            {
                exitRequested_ =
                    true;
            }
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

        const std::wstring input =
            window.ConsumeTextInput();

        if (input.empty())
        {
            return;
        }

        if (focus_ ==
            ui::LoginFocus::Login)
        {
            for (const wchar_t character :
                 input)
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
                        L'\t')
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
                 input)
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

        if (selectedServer_ <
            servers_.size())
        {
            request.server =
                ToUtf8(
                    servers_[
                        selectedServer_]);
        }
        else
        {
            request.server.clear();
        }

        return true;
    }

    bool LoginScreen::ConsumeExitRequest() noexcept
    {
        const bool requested =
            exitRequested_;

        exitRequested_ =
            false;

        return requested;
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
        ui::LoginView view;

        view.login =
            login_;

        view.password =
            password_;

        view.message =
            message_;

        view.servers =
            servers_;

        view.selectedServer =
            selectedServer_;

        view.version =
            version_;

        view.focus =
            focus_;

        view.language =
            language_;

        view.rememberLogin =
            rememberLogin_;

        view.authenticating =
            authenticating_;

        view.serverListOpen =
            serverListOpen_;

        view.canLogin =
            !login_.empty() &&
            !password_.empty();

        if (selectedServer_ <
            servers_.size())
        {
            view.serverName =
                servers_[
                    selectedServer_];
        }

        return view;
    }

    void LoginScreen::LoadServers(
        const core::resources::ResourceFileSystem& resources)
    {
        std::string json;

        if (!resources.ReadText(
                ServersResource,
                json))
        {
            servers_.push_back(
                L"Local Test");

            return;
        }

        std::size_t position =
            0;

        while (true)
        {
            position =
                json.find(
                    "\"name\"",
                    position);

            if (position ==
                std::string::npos)
            {
                break;
            }

            const std::size_t colon =
                json.find(
                    ':',
                    position);

            if (colon ==
                std::string::npos)
            {
                break;
            }

            const std::size_t firstQuote =
                json.find(
                    '"',
                    colon + 1);

            if (firstQuote ==
                std::string::npos)
            {
                break;
            }

            const std::size_t secondQuote =
                json.find(
                    '"',
                    firstQuote + 1);

            if (secondQuote ==
                std::string::npos)
            {
                break;
            }

            const std::string name =
                json.substr(
                    firstQuote + 1,
                    secondQuote -
                        firstQuote -
                        1);

            if (!name.empty())
            {
                servers_.push_back(
                    FromUtf8(
                        name));
            }

            position =
                secondQuote + 1;
        }

        if (servers_.empty())
        {
            servers_.push_back(
                L"Local Test");
        }

        for (std::size_t index = 0;
             index < servers_.size();
             ++index)
        {
            if (servers_[index] ==
                L"Cluster SPB")
            {
                selectedServer_ =
                    index;

                break;
            }
        }
    }

    void LoginScreen::LoadVersion(
        const core::resources::ResourceFileSystem& resources)
    {
        std::vector<std::byte> data;

        if (!resources.ReadBinary(
                VersionResource,
                data))
        {
            version_ =
                L"ver dev";

            return;
        }

        const char marker[] =
            "ver ";

        for (std::size_t index = 0;
             index + 4 < data.size();
             ++index)
        {
            const char c0 =
                static_cast<char>(
                    std::to_integer<
                        unsigned char>(
                            data[index]));

            const char c1 =
                static_cast<char>(
                    std::to_integer<
                        unsigned char>(
                            data[index + 1]));

            const char c2 =
                static_cast<char>(
                    std::to_integer<
                        unsigned char>(
                            data[index + 2]));

            const char c3 =
                static_cast<char>(
                    std::to_integer<
                        unsigned char>(
                            data[index + 3]));

            if (c0 != marker[0] ||
                c1 != marker[1] ||
                c2 != marker[2] ||
                c3 != marker[3])
            {
                continue;
            }

            std::string version;

            for (std::size_t cursor = index;
                 cursor < data.size();
                 ++cursor)
            {
                const unsigned char value =
                    std::to_integer<
                        unsigned char>(
                            data[cursor]);

                if (value < 32 ||
                    value > 126)
                {
                    break;
                }

                version.push_back(
                    static_cast<char>(
                        value));

                if (version.size() >=
                    32)
                {
                    break;
                }
            }

            if (!version.empty())
            {
                version_ =
                    FromUtf8(
                        version);

                return;
            }
        }

        version_ =
            L"ver dev";
    }

    void LoginScreen::Submit()
    {
        serverListOpen_ =
            false;

        message_.clear();

        if (login_.empty())
        {
            message_ =
                Localized(
                    L"Введите имя.",
                    L"Enter your name.");

            focus_ =
                ui::LoginFocus::Login;

            return;
        }

        if (password_.empty())
        {
            message_ =
                Localized(
                    L"Введите пароль.",
                    L"Enter your password.");

            focus_ =
                ui::LoginFocus::Password;

            return;
        }

        submitRequested_ =
            true;
    }

    void LoginScreen::SelectLanguage(
        const ui::FrontendLanguage language)
    {
        language_ =
            language;

        message_.clear();
    }

    std::wstring LoginScreen::Localized(
        const wchar_t* russian,
        const wchar_t* english) const
    {
        if (language_ ==
            ui::FrontendLanguage::Russian)
        {
            return russian;
        }

        return english;
    }

    std::string LoginScreen::ToUtf8(
        const std::wstring& value)
    {
        if (value.empty())
        {
            return {};
        }

        const int length =
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

        if (length <=
            0)
        {
            return {};
        }

        std::string result(
            static_cast<std::size_t>(
                length),
            '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(
                value.size()),
            result.data(),
            length,
            nullptr,
            nullptr);

        return result;
    }

    std::wstring LoginScreen::FromUtf8(
        const std::string& value)
    {
        if (value.empty())
        {
            return {};
        }

        const int length =
            MultiByteToWideChar(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(
                    value.size()),
                nullptr,
                0);

        if (length <=
            0)
        {
            return {};
        }

        std::wstring result(
            static_cast<std::size_t>(
                length),
            L'\0');

        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(
                value.size()),
            result.data(),
            length);

        return result;
    }
}