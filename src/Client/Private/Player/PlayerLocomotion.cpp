#include "Player/PlayerLocomotion.h"

namespace
{
    constexpr float WalkForwardSpeed =
        1.3f;

    constexpr float WalkStrafeSpeed =
        1.3f;

    constexpr float WalkBackwardSpeed =
        1.2f;

    constexpr float RunForwardSpeed =
        3.6f;

    constexpr float RunStrafeSpeed =
        2.7f;

    constexpr float RunBackwardSpeed =
        2.7f;

    constexpr float SprintSpeed =
        6.3f;

    //
    // Во время приседа выбранный Walk / Run / Sprint
    // сохраняется, но физически бег/спринт не включается.
    //
    constexpr float CrouchForwardSpeed =
        1.5f;

    constexpr float CrouchStrafeSpeed =
        1.4f;

    constexpr float CrouchBackwardSpeed =
        1.2f;

    constexpr float StandingCapsuleHeightValue =
        1.78f;

    constexpr float CrouchedCapsuleHeightValue =
        1.20f;

    constexpr float StandingCameraTargetHeightValue =
        1.48f;

    constexpr float CrouchedCameraTargetHeightValue =
        1.05f;

    bool IsForwardDirection(
        const client::player::MovementDirection direction) noexcept
    {
        using client::player::MovementDirection;

        return
            direction ==
                MovementDirection::Forward ||
            direction ==
                MovementDirection::ForwardLeft ||
            direction ==
                MovementDirection::ForwardRight;
    }

    bool IsBackwardDirection(
        const client::player::MovementDirection direction) noexcept
    {
        using client::player::MovementDirection;

        return
            direction ==
                MovementDirection::Backward ||
            direction ==
                MovementDirection::BackwardLeft ||
            direction ==
                MovementDirection::BackwardRight;
    }

    bool IsStrafeDirection(
        const client::player::MovementDirection direction) noexcept
    {
        using client::player::MovementDirection;

        return
            direction ==
                MovementDirection::Left ||
            direction ==
                MovementDirection::Right;
    }
}

namespace client::player
{
    void LocomotionState::Reset() noexcept
    {
        mode_ =
            LocomotionMode::Run;

        crouched_ =
            false;
    }

    void LocomotionState::ToggleWalk() noexcept
    {
        if (mode_ ==
            LocomotionMode::Walk)
        {
            mode_ =
                LocomotionMode::Run;

            return;
        }

        mode_ =
            LocomotionMode::Walk;
    }

    void LocomotionState::ToggleSprint() noexcept
    {
        if (mode_ ==
            LocomotionMode::Sprint)
        {
            mode_ =
                LocomotionMode::Run;

            return;
        }

        mode_ =
            LocomotionMode::Sprint;
    }

    void LocomotionState::SetCrouched(
        const bool crouched) noexcept
    {
        crouched_ =
            crouched;
    }

    LocomotionMode
    LocomotionState::Mode() const noexcept
    {
        return
            mode_;
    }

    LocomotionMode
    LocomotionState::EffectiveMode(
        const MovementDirection direction) const noexcept
    {
        if (mode_ !=
            LocomotionMode::Sprint)
        {
            return
                mode_;
        }

        if (direction ==
            MovementDirection::None)
        {
            return
                LocomotionMode::Sprint;
        }

        //
        // В оригинальном SO Sprint работает только
        // при наличии forward input.
        //
        if (!IsForwardDirection(
                direction))
        {
            return
                LocomotionMode::Run;
        }

        return
            LocomotionMode::Sprint;
    }

    bool LocomotionState::IsCrouched() const noexcept
    {
        return
            crouched_;
    }

    float LocomotionState::Speed(
        const MovementDirection direction) const noexcept
    {
        if (direction ==
            MovementDirection::None)
        {
            return
                0.0f;
        }

        if (crouched_)
        {
            if (IsBackwardDirection(
                    direction))
            {
                return
                    CrouchBackwardSpeed;
            }

            if (IsStrafeDirection(
                    direction))
            {
                return
                    CrouchStrafeSpeed;
            }

            return
                CrouchForwardSpeed;
        }

        const LocomotionMode effectiveMode =
            EffectiveMode(
                direction);

        if (effectiveMode ==
            LocomotionMode::Sprint)
        {
            return
                SprintSpeed;
        }

        if (effectiveMode ==
            LocomotionMode::Walk)
        {
            if (IsBackwardDirection(
                    direction))
            {
                return
                    WalkBackwardSpeed;
            }

            if (IsStrafeDirection(
                    direction))
            {
                return
                    WalkStrafeSpeed;
            }

            return
                WalkForwardSpeed;
        }

        if (IsBackwardDirection(
                direction))
        {
            return
                RunBackwardSpeed;
        }

        if (IsStrafeDirection(
                direction))
        {
            return
                RunStrafeSpeed;
        }

        return
            RunForwardSpeed;
    }

    float LocomotionState::CapsuleHeight() const noexcept
    {
        return
            crouched_
                ? CrouchedCapsuleHeightValue
                : StandingCapsuleHeightValue;
    }

    float LocomotionState::StandingCapsuleHeight() const noexcept
    {
        return
            StandingCapsuleHeightValue;
    }

    float LocomotionState::CameraTargetHeight() const noexcept
    {
        return
            crouched_
                ? CrouchedCameraTargetHeightValue
                : StandingCameraTargetHeightValue;
    }
}