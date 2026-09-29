#pragma once

#include "Graphics/CameraView.h"
#include "World/WorldCollision.h"

#include "Core/Math/Vector3.h"

#include <Windows.h>

namespace client::player
{
    class ThirdPersonCamera final
    {
    public:
        void Reset(
            float yaw) noexcept;

        void UpdateInput(
            HWND window,
            float mouseWheelDelta,
            float deltaSeconds) noexcept;

        void UpdateView(
            const core::math::Vector3& playerPosition,
            float targetHeight,
            const world::Collision& collision) noexcept;

        [[nodiscard]]
        float Yaw() const noexcept;

        [[nodiscard]]
        float ControlYaw() const noexcept;

        [[nodiscard]]
        const graphics::CameraView&
        View() const noexcept;

    private:
        float yaw_ =
            0.0f;

        float controlYaw_ =
            0.0f;

        float pitch_ =
            -0.22f;

        float distance_ =
            4.2f;

        float sensitivity_ =
            0.0025f;

        bool mouseReady_ =
            false;

        bool lookAroundWasDown_ =
            false;

        bool returningFromLookAround_ =
            false;

        graphics::CameraView
            view_{};
    };
}