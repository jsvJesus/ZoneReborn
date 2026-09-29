#pragma once

#include <cstdint>

namespace client::player
{
    enum class TurnDirection : std::uint8_t
    {
        None = 0,
        Left,
        Right
    };

    class TurnController final
    {
    public:
        void Reset(
            float yaw) noexcept;

        void Update(
            float controlYaw,
            float deltaSeconds,
            bool moving,
            bool crouched,
            bool grounded) noexcept;

        void CompleteTurn() noexcept;

        [[nodiscard]]
        float ControlYaw() const noexcept;

        [[nodiscard]]
        float ModelYaw() const noexcept;

        [[nodiscard]]
        float BodyYawOffset() const noexcept;

        [[nodiscard]]
        float FootTwistYaw() const noexcept;

        [[nodiscard]]
        TurnDirection Direction() const noexcept;

        [[nodiscard]]
        int DirectionSign() const noexcept;

    private:
        void UpdateTwist(
            float deltaSeconds) noexcept;

        float controlYaw_ =
            0.0f;

        float modelYaw_ =
            0.0f;

        float bodyYawOffset_ =
            0.0f;

        float footTwistYaw_ =
            0.0f;

        TurnDirection direction_ =
            TurnDirection::None;

        bool crouched_ =
            false;

        bool moving_ =
            false;
    };
}