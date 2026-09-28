#include "Character/CharacterAnimationStateMachine.h"

#include <algorithm>

namespace
{
    constexpr float JumpToFallVelocity =
        0.05f;
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
        if (!input.moving)
        {
            return
                AnimationState::Idle;
        }

        if (input.running)
        {
            return
                AnimationState::Run;
        }

        return
            AnimationState::Walk;
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
        return state_;
    }

    float
    AnimationStateMachine::StateTime() const noexcept
    {
        return stateTime_;
    }
}