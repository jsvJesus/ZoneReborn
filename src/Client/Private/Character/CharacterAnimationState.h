#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace client::character
{
    enum class AnimationState : std::uint8_t
    {
        Idle = 0,
        Walk,
        Run,
        Jump,
        Fall,
        Land,

        Count
    };

    inline constexpr std::size_t AnimationStateCount =
        static_cast<std::size_t>(
            AnimationState::Count);

    [[nodiscard]]
    inline constexpr std::size_t
    AnimationStateIndex(
        const AnimationState state) noexcept
    {
        return
            static_cast<std::size_t>(
                state);
    }

    [[nodiscard]]
    inline constexpr std::string_view
    AnimationStateName(
        const AnimationState state) noexcept
    {
        switch (state)
        {
        case AnimationState::Idle:
            return "Idle";

        case AnimationState::Walk:
            return "Walk";

        case AnimationState::Run:
            return "Run";

        case AnimationState::Jump:
            return "Jump";

        case AnimationState::Fall:
            return "Fall";

        case AnimationState::Land:
            return "Land";

        case AnimationState::Count:
            break;
        }

        return "Unknown";
    }

    [[nodiscard]]
    inline constexpr bool
    AnimationStateLoops(
        const AnimationState state) noexcept
    {
        switch (state)
        {
        case AnimationState::Idle:
        case AnimationState::Walk:
        case AnimationState::Run:
        case AnimationState::Fall:
            return true;

        case AnimationState::Jump:
        case AnimationState::Land:
        case AnimationState::Count:
            return false;
        }

        return false;
    }
}