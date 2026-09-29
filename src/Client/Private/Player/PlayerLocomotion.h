#pragma once

#include <cstdint>

namespace client::player
{
    enum class LocomotionMode : std::uint8_t
    {
        Walk = 0,
        Run,
        Sprint
    };

    class LocomotionState final
    {
    public:
        void Reset() noexcept;

        void ToggleWalk() noexcept;

        void ToggleSprint() noexcept;

        void SetCrouched(
            bool crouched) noexcept;

        [[nodiscard]]
        LocomotionMode Mode() const noexcept;

        [[nodiscard]]
        bool IsCrouched() const noexcept;

        [[nodiscard]]
        float Speed() const noexcept;

        [[nodiscard]]
        float CapsuleHeight() const noexcept;

        [[nodiscard]]
        float StandingCapsuleHeight() const noexcept;

        [[nodiscard]]
        float CameraTargetHeight() const noexcept;

    private:
        LocomotionMode mode_ =
            LocomotionMode::Run;

        bool crouched_ =
            false;
    };
}