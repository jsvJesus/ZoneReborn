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

    enum class MovementDirection : std::uint8_t
    {
        None = 0,

        Forward,
        ForwardRight,
        Right,
        BackwardRight,
        Backward,
        BackwardLeft,
        Left,
        ForwardLeft
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
        LocomotionMode EffectiveMode(
            MovementDirection direction) const noexcept;

        [[nodiscard]]
        bool IsCrouched() const noexcept;

        [[nodiscard]]
        float Speed(
            MovementDirection direction) const noexcept;

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