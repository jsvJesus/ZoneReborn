#pragma once

#include <string>

namespace client::ui
{
    struct Rect final
    {
        float left =
            0.0f;

        float top =
            0.0f;

        float right =
            0.0f;

        float bottom =
            0.0f;

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

    struct LoginView final
    {
        std::wstring login;
        std::wstring password;

        std::wstring message;

        LoginFocus focus =
            LoginFocus::None;

        bool rememberLogin =
            false;

        bool authenticating =
            false;
    };
}