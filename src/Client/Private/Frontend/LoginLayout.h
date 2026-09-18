#pragma once

#include "UI/FrontendView.h"

#include <cstddef>

namespace client::frontend::layout
{
    inline constexpr float
        ReferenceWidth =
            1280.0f;

    inline constexpr float
        ReferenceHeight =
            768.0f;

    inline constexpr ui::Rect LoginPanel
    {
        407.0f,
        234.0f,
        868.0f,
        585.0f
    };

    inline constexpr ui::Rect RegisterLink
    {
        610.0f,
        244.0f,
        775.0f,
        281.0f
    };

    inline constexpr ui::Rect ServerValue
    {
        547.0f,
        310.0f,
        807.0f,
        352.0f
    };

    inline constexpr ui::Rect ServerButton
    {
        807.0f,
        310.0f,
        847.0f,
        352.0f
    };

    inline constexpr ui::Rect LoginEdit
    {
        547.0f,
        381.0f,
        847.0f,
        422.0f
    };

    inline constexpr ui::Rect PasswordEdit
    {
        547.0f,
        431.0f,
        847.0f,
        472.0f
    };

    inline constexpr ui::Rect RememberToggle
    {
        797.0f,
        489.0f,
        848.0f,
        516.0f
    };

    inline constexpr ui::Rect LoginButton
    {
        407.0f,
        524.0f,
        868.0f,
        585.0f
    };

    inline constexpr ui::Rect SettingsButton
    {
        1084.0f,
        8.0f,
        1118.0f,
        43.0f
    };

    inline constexpr ui::Rect EnglishButton
    {
        1135.0f,
        10.0f,
        1168.0f,
        43.0f
    };

    inline constexpr ui::Rect RussianButton
    {
        1174.0f,
        10.0f,
        1208.0f,
        43.0f
    };

    inline constexpr ui::Rect ExitButton
    {
        1231.0f,
        8.0f,
        1263.0f,
        43.0f
    };

    [[nodiscard]]
    inline ui::Rect ServerListItem(
        const std::size_t index)
    {
        const float top =
            310.0f +
            static_cast<float>(
                index) *
            36.0f;

        return
        {
            875.0f,
            top,
            1120.0f,
            top + 34.0f
        };
    }
}