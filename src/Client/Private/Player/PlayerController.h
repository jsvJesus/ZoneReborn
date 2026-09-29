#pragma once

#include "Player/PlayerLocomotion.h"

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
            float cameraYaw,
            const world::Collision& collision) noexcept;

        [[nodiscard]]
        const core::math::Vector3&
        Position() const noexcept;

        [[nodiscard]]
        float Yaw() const noexcept;

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

        void MoveHorizontal(
            const core::math::Vector3& movement,
            const world::Collision& collision) noexcept;

        core::math::Vector3
            position_{};

        LocomotionState
            locomotion_;

        float yaw_ =
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