#include "Character/CharacterAnimationStateMachine.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float JumpToFallVelocity =
        0.05f;

    client::character::AnimationState WalkingState(
        const float forward,
        const float right) noexcept
    {
        using client::character::AnimationState;

        if (std::abs(
                forward) >=
            std::abs(
                right))
        {
            if (forward <
                0.0f)
            {
                return
                    AnimationState::WalkBackward;
            }

            return
                AnimationState::WalkForward;
        }

        return
            right <
                0.0f
                ? AnimationState::WalkStrafeLeft
                : AnimationState::WalkStrafeRight;
    }

    client::character::AnimationState RunningState(
        const float forward,
        const float right) noexcept
    {
        using client::character::AnimationState;

        if (std::abs(
                forward) >=
            std::abs(
                right))
        {
            if (forward <
                0.0f)
            {
                return
                    AnimationState::RunBackward;
            }

            return
                AnimationState::RunForward;
        }

        return
            right <
                0.0f
                ? AnimationState::RunStrafeLeft
                : AnimationState::RunStrafeRight;
    }

    client::character::AnimationState CrouchedState(
        const float forward,
        const float right) noexcept
    {
        using client::character::AnimationState;

        if (std::abs(
                forward) >=
            std::abs(
                right))
        {
            if (forward <
                0.0f)
            {
                return
                    AnimationState::CrouchBackward;
            }

            return
                AnimationState::CrouchForward;
        }

        return
            right <
                0.0f
                ? AnimationState::CrouchStrafeLeft
                : AnimationState::CrouchStrafeRight;
    }
}

namespace client::character
{
    void AnimationStateMachine::Reset(
        const bool grounded) noexcept
    {
        state_ =
            grounded
                ? AnimationState::Idle
                : AnimationState::Fall;

        stateTime_ =
            0.0f;

        previousGrounded_ =
            grounded;
    }

    AnimationState
    AnimationStateMachine::GroundState(
        const AnimationInput& input) noexcept
    {
        if (input.crouched)
        {
            if (input.moving)
            {
                return
                    CrouchedState(
                        input.moveForward,
                        input.moveRight);
            }

            if (input.turnDirection <
                0)
            {
                return
                    AnimationState::CrouchTurnLeft;
            }

            if (input.turnDirection >
                0)
            {
                return
                    AnimationState::CrouchTurnRight;
            }

            return
                AnimationState::CrouchIdle;
        }

        if (!input.moving)
        {
            if (input.turnDirection <
                0)
            {
                return
                    AnimationState::TurnLeft;
            }

            if (input.turnDirection >
                0)
            {
                return
                    AnimationState::TurnRight;
            }

            return
                AnimationState::Idle;
        }

        if (input.sprinting &&
            input.moveForward >
                0.0f)
        {
            return
                AnimationState::Sprint;
        }

        if (input.walking)
        {
            return
                WalkingState(
                    input.moveForward,
                    input.moveRight);
        }

        return
            RunningState(
                input.moveForward,
                input.moveRight);
    }

    void AnimationStateMachine::ChangeState(
        const AnimationState state) noexcept
    {
        if (state_ ==
            state)
        {
            return;
        }

        state_ =
            state;

        stateTime_ =
            0.0f;
    }

    void AnimationStateMachine::Update(
        float deltaSeconds,
        const AnimationInput& input,
        const bool activeClipFinished) noexcept
    {
        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        const bool justLanded =
            input.grounded &&
            !previousGrounded_;

        AnimationState target =
            state_;

        if (!input.grounded)
        {
            target =
                input.verticalVelocity >
                    JumpToFallVelocity
                    ? AnimationState::Jump
                    : AnimationState::Fall;
        }
        else if (justLanded)
        {
            target =
                AnimationState::Land;
        }
        else if (state_ ==
                     AnimationState::Land &&
                 !activeClipFinished)
        {
            target =
                AnimationState::Land;
        }
        else
        {
            target =
                GroundState(
                    input);
        }

        if (target !=
            state_)
        {
            ChangeState(
                target);
        }
        else
        {
            stateTime_ +=
                deltaSeconds;
        }

        previousGrounded_ =
            input.grounded;
    }

    AnimationState
    AnimationStateMachine::Current() const noexcept
    {
        return
            state_;
    }

    float
    AnimationStateMachine::StateTime() const noexcept
    {
        return
            stateTime_;
    }
}