#pragma once

#include "Character/CharacterAnimationState.h"

namespace client::character
{
    struct AnimationInput final
    {
        bool moving =
            false;

        bool walking =
            false;

        bool sprinting =
            false;

        bool crouched =
            false;

        bool grounded =
            true;

        float moveForward =
            0.0f;

        float moveRight =
            0.0f;

        int turnDirection =
            0;

        float verticalVelocity =
            0.0f;
    };

    class AnimationStateMachine final
    {
    public:
        void Reset(
            bool grounded) noexcept;

        void Update(
            float deltaSeconds,
            const AnimationInput& input,
            bool activeClipFinished) noexcept;

        [[nodiscard]]
        AnimationState Current() const noexcept;

        [[nodiscard]]
        float StateTime() const noexcept;

    private:
        void ChangeState(
            AnimationState state) noexcept;

        [[nodiscard]]
        static AnimationState GroundState(
            const AnimationInput& input) noexcept;

        AnimationState state_ =
            AnimationState::Idle;

        float stateTime_ =
            0.0f;

        bool previousGrounded_ =
            true;
    };
}