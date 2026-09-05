#include "Input/CameraController.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    constexpr float MinimumPitch =
        -89.0f *
        Pi /
        180.0f;

    constexpr float MaximumPitch =
        89.0f *
        Pi /
        180.0f;

    constexpr float MinimumFov =
        20.0f;

    constexpr float MaximumFov =
        90.0f;

    constexpr float MinimumSpeed =
        1.0f;

    constexpr float MaximumSpeed =
        1000.0f;

    core::math::Vector3 Add(
        const core::math::Vector3& left,
        const core::math::Vector3& right) noexcept
    {
        return
        {
            left.x + right.x,
            left.y + right.y,
            left.z + right.z
        };
    }

    core::math::Vector3 Scale(
        const core::math::Vector3& value,
        const float scale) noexcept
    {
        return
        {
            value.x * scale,
            value.y * scale,
            value.z * scale
        };
    }
}

namespace client::input
{
    bool CameraController::IsKeyDown(
        const int virtualKey) noexcept
    {
        return
            (
                GetAsyncKeyState(
                    virtualKey) &
                0x8000
            ) != 0;
    }

    void CameraController::Reset(
        const core::math::Vector3& sceneCenter,
        const float sceneRadius) noexcept
    {
        const float distance =
            std::max(
                sceneRadius *
                    0.35f,
                80.0f);

        const float height =
            std::max(
                sceneRadius *
                    0.12f,
                25.0f);

        position_ =
        {
            sceneCenter.x,
            sceneCenter.y +
                height,
            sceneCenter.z -
                distance
        };

        yaw_ =
            0.0f;

        pitch_ =
            -std::atan2(
                height,
                distance);

        resetPosition_ =
            position_;

        resetYaw_ =
            yaw_;

        resetPitch_ =
            pitch_;

        movementSpeed_ =
            std::clamp(
                sceneRadius *
                    0.12f,
                15.0f,
                60.0f);

        fieldOfViewDegrees_ =
            60.0f;

        mouseLookActive_ =
            false;

        speedIncreaseWasDown_ =
            false;

        speedDecreaseWasDown_ =
            false;

        resetWasDown_ =
            false;

        RebuildView();
    }

    void CameraController::Update(
        const HWND window,
        const float mouseWheelDelta,
        float deltaSeconds) noexcept
    {
        if (window == nullptr)
        {
            return;
        }

        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.1f);

        if (GetForegroundWindow() !=
            window)
        {
            mouseLookActive_ =
                false;

            return;
        }

        const bool altDown =
            IsKeyDown(
                VK_MENU);

        if (mouseWheelDelta !=
            0.0f)
        {
            if (altDown)
            {
                const float factor =
                    std::pow(
                        1.25f,
                        mouseWheelDelta);

                movementSpeed_ =
                    std::clamp(
                        movementSpeed_ *
                            factor,
                        MinimumSpeed,
                        MaximumSpeed);
            }
            else
            {
                fieldOfViewDegrees_ =
                    std::clamp(
                        fieldOfViewDegrees_ -
                            mouseWheelDelta *
                                5.0f,
                        MinimumFov,
                        MaximumFov);
            }
        }

        const bool speedIncreaseDown =
            IsKeyDown(
                VK_OEM_PLUS) ||
            IsKeyDown(
                VK_ADD);

        if (speedIncreaseDown &&
            !speedIncreaseWasDown_)
        {
            movementSpeed_ =
                std::clamp(
                    movementSpeed_ *
                        1.25f,
                    MinimumSpeed,
                    MaximumSpeed);
        }

        speedIncreaseWasDown_ =
            speedIncreaseDown;

        const bool speedDecreaseDown =
            IsKeyDown(
                VK_OEM_MINUS) ||
            IsKeyDown(
                VK_SUBTRACT);

        if (speedDecreaseDown &&
            !speedDecreaseWasDown_)
        {
            movementSpeed_ =
                std::clamp(
                    movementSpeed_ /
                        1.25f,
                    MinimumSpeed,
                    MaximumSpeed);
        }

        speedDecreaseWasDown_ =
            speedDecreaseDown;

        const bool resetDown =
            IsKeyDown(
                VK_HOME);

        if (resetDown &&
            !resetWasDown_)
        {
            position_ =
                resetPosition_;

            yaw_ =
                resetYaw_;

            pitch_ =
                resetPitch_;

            fieldOfViewDegrees_ =
                60.0f;
        }

        resetWasDown_ =
            resetDown;

        const bool rightMouseDown =
            IsKeyDown(
                VK_RBUTTON);

        if (rightMouseDown)
        {
            RECT clientRectangle{};

            if (GetClientRect(
                    window,
                    &clientRectangle))
            {
                POINT center
                {
                    (
                        clientRectangle.right -
                        clientRectangle.left
                    ) /
                        2,

                    (
                        clientRectangle.bottom -
                        clientRectangle.top
                    ) /
                        2
                };

                ClientToScreen(
                    window,
                    &center);

                if (!mouseLookActive_)
                {
                    mouseLookActive_ =
                        true;

                    SetCapture(
                        window);

                    SetCursorPos(
                        center.x,
                        center.y);
                }
                else
                {
                    POINT cursor{};

                    if (GetCursorPos(
                            &cursor))
                    {
                        const LONG deltaX =
                            cursor.x -
                            center.x;

                        const LONG deltaY =
                            cursor.y -
                            center.y;

                        yaw_ +=
                            static_cast<float>(
                                deltaX) *
                            mouseSensitivity_;

                        pitch_ -=
                            static_cast<float>(
                                deltaY) *
                            mouseSensitivity_;

                        pitch_ =
                            std::clamp(
                                pitch_,
                                MinimumPitch,
                                MaximumPitch);

                        if (deltaX != 0 ||
                            deltaY != 0)
                        {
                            SetCursorPos(
                                center.x,
                                center.y);
                        }
                    }
                }
            }
        }
        else if (mouseLookActive_)
        {
            mouseLookActive_ =
                false;

            if (GetCapture() ==
                window)
            {
                ReleaseCapture();
            }
        }

        const float cosPitch =
            std::cos(
                pitch_);

        const float sinPitch =
            std::sin(
                pitch_);

        const float sinYaw =
            std::sin(
                yaw_);

        const float cosYaw =
            std::cos(
                yaw_);

        const core::math::Vector3 forward
        {
            cosPitch *
                sinYaw,

            sinPitch,

            cosPitch *
                cosYaw
        };

        const core::math::Vector3 right
        {
            cosYaw,
            0.0f,
            -sinYaw
        };

        float speed =
            movementSpeed_;

        if (IsKeyDown(
                VK_SHIFT))
        {
            speed *=
                4.0f;
        }

        const float movement =
            speed *
            deltaSeconds;

        if (IsKeyDown('W'))
        {
            position_ =
                Add(
                    position_,
                    Scale(
                        forward,
                        movement));
        }

        if (IsKeyDown('S'))
        {
            position_ =
                Add(
                    position_,
                    Scale(
                        forward,
                        -movement));
        }

        if (IsKeyDown('D'))
        {
            position_ =
                Add(
                    position_,
                    Scale(
                        right,
                        movement));
        }

        if (IsKeyDown('A'))
        {
            position_ =
                Add(
                    position_,
                    Scale(
                        right,
                        -movement));
        }

        if (IsKeyDown(
                VK_SPACE) ||
            IsKeyDown('E'))
        {
            position_.y +=
                movement;
        }

        if (IsKeyDown(
                VK_CONTROL) ||
            IsKeyDown('Q'))
        {
            position_.y -=
                movement;
        }

        RebuildView();
    }

    void CameraController::RebuildView() noexcept
    {
        const float cosPitch =
            std::cos(
                pitch_);

        view_.position =
            position_;

        view_.forward =
        {
            cosPitch *
                std::sin(
                    yaw_),

            std::sin(
                pitch_),

            cosPitch *
                std::cos(
                    yaw_)
        };

        view_.up =
        {
            0.0f,
            1.0f,
            0.0f
        };

        view_.fieldOfViewDegrees =
            fieldOfViewDegrees_;
    }

    const graphics::CameraView&
    CameraController::View() const noexcept
    {
        return view_;
    }

    float CameraController::MovementSpeed() const noexcept
    {
        return movementSpeed_;
    }
}