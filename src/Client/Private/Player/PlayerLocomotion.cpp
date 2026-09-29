#include "Player/PlayerLocomotion.h"

namespace
{
    constexpr float WalkSpeed =
        1.3f;

    constexpr float RunSpeed =
        3.6f;

    constexpr float SprintSpeed =
        6.3f;

    constexpr float CrouchSpeed =
        1.5f;

    constexpr float StandingCapsuleHeight =
        1.78f;

    constexpr float CrouchedCapsuleHeight =
        1.20f;

    constexpr float StandingCameraTargetHeight =
        1.48f;

    constexpr float CrouchedCameraTargetHeight =
        1.05f;
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

    bool LocomotionState::IsCrouched() const noexcept
    {
        return
            crouched_;
    }

    float LocomotionState::Speed() const noexcept
    {
        if (crouched_)
        {
            return
                CrouchSpeed;
        }

        switch (mode_)
        {
        case LocomotionMode::Walk:
            return
                WalkSpeed;

        case LocomotionMode::Run:
            return
                RunSpeed;

        case LocomotionMode::Sprint:
            return
                SprintSpeed;
        }

        return
            RunSpeed;
    }

    float LocomotionState::CapsuleHeight() const noexcept
    {
        return
            crouched_
                ? CrouchedCapsuleHeight
                : StandingCapsuleHeight;
    }

    float LocomotionState::StandingCapsuleHeight() const noexcept
    {
        return
            ::StandingCapsuleHeight;
    }

    float LocomotionState::CameraTargetHeight() const noexcept
    {
        return
            crouched_
                ? CrouchedCameraTargetHeight
                : StandingCameraTargetHeight;
    }
}