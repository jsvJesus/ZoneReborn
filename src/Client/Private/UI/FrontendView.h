#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace client::ui
{
    struct Rect final
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        [[nodiscard]]
        bool Contains(
            const float x,
            const float y) const noexcept
        {
            return
                x >= left &&
                x <= right &&
                y >= top &&
                y <= bottom;
        }
    };

    enum class LoginFocus
    {
        None,
        Login,
        Password
    };

    enum class FrontendLanguage
    {
        Russian,
        English
    };

    struct LoginView final
    {
        std::wstring login;
        std::wstring password;

        std::wstring message;

        std::wstring serverName;
        std::wstring version;

        std::vector<std::wstring>
            servers;

        std::size_t selectedServer =
            0;

        LoginFocus focus =
            LoginFocus::None;

        FrontendLanguage language =
            FrontendLanguage::Russian;

        bool rememberLogin =
            false;

        bool authenticating =
            false;

        bool serverListOpen =
            false;

        bool canLogin =
            false;
    };
}