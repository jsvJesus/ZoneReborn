#include "Player/PlayerLocomotion.h"

namespace
{
    constexpr float WalkSpeedValue =
        1.3f;

    constexpr float RunSpeedValue =
        3.6f;

    constexpr float SprintSpeedValue =
        6.3f;

    constexpr float CrouchSpeedValue =
        1.5f;

    constexpr float StandingCapsuleHeightValue =
        1.78f;

    constexpr float CrouchedCapsuleHeightValue =
        1.20f;

    constexpr float StandingCameraTargetHeightValue =
        1.48f;

    constexpr float CrouchedCameraTargetHeightValue =
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
                CrouchSpeedValue;
        }

        switch (mode_)
        {
        case LocomotionMode::Walk:
            return
                WalkSpeedValue;

        case LocomotionMode::Run:
            return
                RunSpeedValue;

        case LocomotionMode::Sprint:
            return
                SprintSpeedValue;
        }

        return
            RunSpeedValue;
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