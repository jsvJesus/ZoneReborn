#pragma once

#include "Graphics/CameraView.h"

#include <Windows.h>

namespace client::input
{
    class CameraController final
    {
    public:
        void Reset(
            const core::math::Vector3& sceneCenter,
            float sceneRadius) noexcept;

        void Update(
            HWND window,
            float mouseWheelDelta,
            float deltaSeconds) noexcept;

        [[nodiscard]]
        const graphics::CameraView& View() const noexcept;

        [[nodiscard]]
        float MovementSpeed() const noexcept;

    private:
        void RebuildView() noexcept;

        [[nodiscard]]
        static bool IsKeyDown(
            int virtualKey) noexcept;

        core::math::Vector3 position_{};

        core::math::Vector3 resetPosition_{};

        float yaw_ = 0.0f;
        float pitch_ = 0.0f;

        float resetYaw_ = 0.0f;
        float resetPitch_ = 0.0f;

        float movementSpeed_ =
            30.0f;

        float fieldOfViewDegrees_ =
            60.0f;

        float mouseSensitivity_ =
            0.0025f;

        bool mouseLookActive_ =
            false;

        bool speedIncreaseWasDown_ =
            false;

        bool speedDecreaseWasDown_ =
            false;

        bool resetWasDown_ =
            false;

        graphics::CameraView view_{};
    };
}