#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace client::character
{
    enum class AnimationState : std::uint8_t
    {
        Idle = 0,

        WalkForward,
        WalkBackward,
        WalkStrafeLeft,
        WalkStrafeRight,

        RunForward,
        RunBackward,
        RunStrafeLeft,
        RunStrafeRight,

        Sprint,

        CrouchIdle,
        CrouchForward,
        CrouchBackward,
        CrouchStrafeLeft,
        CrouchStrafeRight,

        TurnLeft,
        TurnRight,

        CrouchTurnLeft,
        CrouchTurnRight,

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
    inline constexpr bool
    AnimationStateIsTurn(
        const AnimationState state) noexcept
    {
        return
            state ==
                AnimationState::TurnLeft ||
            state ==
                AnimationState::TurnRight ||
            state ==
                AnimationState::CrouchTurnLeft ||
            state ==
                AnimationState::CrouchTurnRight;
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

        case AnimationState::WalkForward:
            return "WalkForward";

        case AnimationState::WalkBackward:
            return "WalkBackward";

        case AnimationState::WalkStrafeLeft:
            return "WalkStrafeLeft";

        case AnimationState::WalkStrafeRight:
            return "WalkStrafeRight";

        case AnimationState::RunForward:
            return "RunForward";

        case AnimationState::RunBackward:
            return "RunBackward";

        case AnimationState::RunStrafeLeft:
            return "RunStrafeLeft";

        case AnimationState::RunStrafeRight:
            return "RunStrafeRight";

        case AnimationState::Sprint:
            return "Sprint";

        case AnimationState::CrouchIdle:
            return "CrouchIdle";

        case AnimationState::CrouchForward:
            return "CrouchForward";

        case AnimationState::CrouchBackward:
            return "CrouchBackward";

        case AnimationState::CrouchStrafeLeft:
            return "CrouchStrafeLeft";

        case AnimationState::CrouchStrafeRight:
            return "CrouchStrafeRight";

        case AnimationState::TurnLeft:
            return "TurnLeft";

        case AnimationState::TurnRight:
            return "TurnRight";

        case AnimationState::CrouchTurnLeft:
            return "CrouchTurnLeft";

        case AnimationState::CrouchTurnRight:
            return "CrouchTurnRight";

        case AnimationState::Jump:
            return "Jump";

        case AnimationState::Fall:
            return "Fall";

        case AnimationState::Land:
            return "Land";

        case AnimationState::Count:
            break;
        }

        return
            "Unknown";
    }

    [[nodiscard]]
    inline constexpr bool
    AnimationStateLoops(
        const AnimationState state) noexcept
    {
        switch (state)
        {
        case AnimationState::Idle:

        case AnimationState::WalkForward:
        case AnimationState::WalkBackward:
        case AnimationState::WalkStrafeLeft:
        case AnimationState::WalkStrafeRight:

        case AnimationState::RunForward:
        case AnimationState::RunBackward:
        case AnimationState::RunStrafeLeft:
        case AnimationState::RunStrafeRight:

        case AnimationState::Sprint:

        case AnimationState::CrouchIdle:
        case AnimationState::CrouchForward:
        case AnimationState::CrouchBackward:
        case AnimationState::CrouchStrafeLeft:
        case AnimationState::CrouchStrafeRight:

        case AnimationState::Fall:
            return
                true;

        case AnimationState::TurnLeft:
        case AnimationState::TurnRight:
        case AnimationState::CrouchTurnLeft:
        case AnimationState::CrouchTurnRight:

        case AnimationState::Jump:
        case AnimationState::Land:

        case AnimationState::Count:
            return
                false;
        }

        return
            false;
    }
}