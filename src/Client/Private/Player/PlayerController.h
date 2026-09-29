#pragma once

#include "Player/PlayerLocomotion.h"
#include "Player/PlayerTurnController.h"

#include "World/WorldCollision.h"

#include "Core/Math/Transform3x4.h"
#include "Core/Math/Vector3.h"

#include <Windows.h>

namespace client::player
{
    class Controller final
    {
    public:
        void Reset(
            const core::math::Vector3& position,
            float yaw) noexcept;

        void Update(
            HWND window,
            float deltaSeconds,
            float controlYaw,
            const world::Collision& collision) noexcept;

        void CompleteTurn() noexcept;

        [[nodiscard]]
        const core::math::Vector3&
        Position() const noexcept;

        [[nodiscard]]
        float Yaw() const noexcept;

        [[nodiscard]]
        float ModelYaw() const noexcept;

        [[nodiscard]]
        float BodyYawOffset() const noexcept;

        [[nodiscard]]
        float FootTwistYaw() const noexcept;

        [[nodiscard]]
        int TurnDirectionSign() const noexcept;

        [[nodiscard]]
        float MoveForwardInput() const noexcept;

        [[nodiscard]]
        float MoveRightInput() const noexcept;

        [[nodiscard]]
        MovementDirection Direction() const noexcept;

        [[nodiscard]]
        bool IsMoving() const noexcept;

        [[nodiscard]]
        LocomotionMode SelectedLocomotionMode() const noexcept;

        [[nodiscard]]
        bool IsWalking() const noexcept;

        [[nodiscard]]
        bool IsRunning() const noexcept;

        [[nodiscard]]
        bool IsSprinting() const noexcept;

        [[nodiscard]]
        bool IsCrouched() const noexcept;

        [[nodiscard]]
        bool IsGrounded() const noexcept;

        [[nodiscard]]
        float VerticalVelocity() const noexcept;

        [[nodiscard]]
        float CameraTargetHeight() const noexcept;

        [[nodiscard]]
        core::math::Transform3x4
        Transform() const noexcept;

    private:
        [[nodiscard]]
        static bool IsKeyDown(
            int key) noexcept;

        [[nodiscard]]
        static MovementDirection ResolveDirection(
            float right,
            float forward) noexcept;

        void MoveHorizontal(
            const core::math::Vector3& movement,
            const world::Collision& collision) noexcept;

        core::math::Vector3
            position_{};

        LocomotionState
            locomotion_;

        TurnController
            turn_;

        MovementDirection movementDirection_ =
            MovementDirection::None;

        float moveForwardInput_ =
            0.0f;

        float moveRightInput_ =
            0.0f;

        float verticalVelocity_ =
            0.0f;

        bool grounded_ =
            true;

        bool moving_ =
            false;

        bool sprintWasDown_ =
            false;

        bool crouchWasDown_ =
            false;

        bool walkWasDown_ =
            false;

        bool jumpWasDown_ =
            false;
    };
}