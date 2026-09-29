#include "Player/ThirdPersonCamera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    constexpr float MinimumPitch =
        -55.0f *
        Pi /
        180.0f;

    constexpr float MaximumPitch =
        25.0f *
        Pi /
        180.0f;

    constexpr float MinimumDistance =
        2.0f;

    constexpr float MaximumDistance =
        6.5f;

    core::math::Vector3 Add(
        const core::math::Vector3& a,
        const core::math::Vector3& b) noexcept
    {
        return
        {
            a.x + b.x,
            a.y + b.y,
            a.z + b.z
        };
    }

    core::math::Vector3 Subtract(
        const core::math::Vector3& a,
        const core::math::Vector3& b) noexcept
    {
        return
        {
            a.x - b.x,
            a.y - b.y,
            a.z - b.z
        };
    }

    core::math::Vector3 Multiply(
        const core::math::Vector3& value,
        const float scalar) noexcept
    {
        return
        {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    core::math::Vector3 Normalize(
        const core::math::Vector3& value) noexcept
    {
        const float length =
            std::sqrt(
                value.x * value.x +
                value.y * value.y +
                value.z * value.z);

        if (length <=
            0.000001f)
        {
            return
            {
                0.0f,
                0.0f,
                1.0f
            };
        }

        return
        {
            value.x / length,
            value.y / length,
            value.z / length
        };
    }
}

namespace client::player
{
    constexpr float LookAroundReturnResponse =
        9.0f;

    float NormalizeAngle(
        float angle) noexcept
    {
        while (angle >
               Pi)
        {
            angle -=
                Pi *
                2.0f;
        }

        while (angle <
               -Pi)
        {
            angle +=
                Pi *
                2.0f;
        }

        return
            angle;
    }
    
    void ThirdPersonCamera::Reset(
        const float yaw) noexcept
    {
        yaw_ =
            NormalizeAngle(
                yaw);

        controlYaw_ =
            yaw_;

        pitch_ =
            -0.22f;

        distance_ =
            4.2f;

        mouseReady_ =
            false;

        lookAroundWasDown_ =
            false;

        returningFromLookAround_ =
            false;
    }

    void ThirdPersonCamera::UpdateInput(
        const HWND window,
        const float mouseWheelDelta,
        float deltaSeconds) noexcept
    {
        if (window ==
            nullptr)
        {
            return;
        }

        if (GetForegroundWindow() !=
            window)
        {
            mouseReady_ =
                false;

            return;
        }

        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        if (mouseWheelDelta !=
            0.0f)
        {
            distance_ =
                std::clamp(
                    distance_ -
                        mouseWheelDelta *
                        0.4f,
                    MinimumDistance,
                    MaximumDistance);
        }

        const bool lookAroundDown =
            (
                GetAsyncKeyState(
                    VK_MENU) &
                0x8000
            ) != 0;

        if (lookAroundWasDown_ &&
            !lookAroundDown)
        {
            returningFromLookAround_ =
                true;
        }

        if (lookAroundDown)
        {
            returningFromLookAround_ =
                false;
        }

        RECT rectangle{};

        if (!GetClientRect(
                window,
                &rectangle))
        {
            return;
        }

        POINT center
        {
            (
                rectangle.right -
                rectangle.left
            ) /
            2,

            (
                rectangle.bottom -
                rectangle.top
            ) /
            2
        };

        ClientToScreen(
            window,
            &center);

        if (!mouseReady_)
        {
            SetCursorPos(
                center.x,
                center.y);

            mouseReady_ =
                true;

            lookAroundWasDown_ =
                lookAroundDown;

            return;
        }

        POINT cursor{};

        if (!GetCursorPos(
                &cursor))
        {
            return;
        }

        const LONG deltaX =
            cursor.x -
            center.x;

        const LONG deltaY =
            cursor.y -
            center.y;

        const float yawDelta =
            static_cast<float>(
                deltaX) *
            sensitivity_;

        yaw_ =
            NormalizeAngle(
                yaw_ +
                yawDelta);

        pitch_ -=
            static_cast<float>(
                deltaY) *
            sensitivity_;

        pitch_ =
            std::clamp(
                pitch_,
                MinimumPitch,
                MaximumPitch);

        if (lookAroundDown)
        {
            //
            // ALT:
            // камера вращается отдельно,
            // направление персонажа не меняется.
            //
        }
        else if (returningFromLookAround_)
        {
            if (deltaX !=
                0)
            {
                //
                // Игрок снова начал крутить мышь:
                // прекращаем автоматический возврат.
                //
                returningFromLookAround_ =
                    false;

                controlYaw_ =
                    yaw_;
            }
            else
            {
                const float difference =
                    NormalizeAngle(
                        controlYaw_ -
                        yaw_);

                const float factor =
                    1.0f -
                    std::exp(
                        -LookAroundReturnResponse *
                        deltaSeconds);

                yaw_ =
                    NormalizeAngle(
                        yaw_ +
                        difference *
                        factor);

                if (std::abs(
                        difference) <
                    0.001f)
                {
                    yaw_ =
                        controlYaw_;

                    returningFromLookAround_ =
                        false;
                }
            }
        }
        else
        {
            controlYaw_ =
                yaw_;
        }

        lookAroundWasDown_ =
            lookAroundDown;

        if (deltaX != 0 ||
            deltaY != 0)
        {
            SetCursorPos(
                center.x,
                center.y);
        }
    }

    void ThirdPersonCamera::UpdateView(
        const core::math::Vector3& playerPosition,
        const float targetHeight,
        const world::Collision& collision) noexcept
    {
        const core::math::Vector3 target
        {
            playerPosition.x,
            playerPosition.y +
                targetHeight,
            playerPosition.z
        };

        const float cosPitch =
            std::cos(
                pitch_);

        const core::math::Vector3 forward
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

        const core::math::Vector3 right
        {
            std::cos(
                yaw_),

            0.0f,

            -std::sin(
                yaw_)
        };

        core::math::Vector3 desired =
            Add(
                Subtract(
                    target,
                    Multiply(
                        forward,
                        distance_)),
                Multiply(
                    right,
                    0.35f));

        float hitFraction =
            1.0f;

        if (collision.Raycast(
                target,
                desired,
                hitFraction))
        {
            hitFraction =
                std::max(
                    0.08f,
                    hitFraction -
                        0.04f);

            desired =
                Add(
                    target,
                    Multiply(
                        Subtract(
                            desired,
                            target),
                        hitFraction));
        }

        view_.position =
            desired;

        view_.forward =
            Normalize(
                Subtract(
                    target,
                    desired));

        view_.up =
        {
            0.0f,
            1.0f,
            0.0f
        };

        view_.fieldOfViewDegrees =
            65.0f;
    }

    float ThirdPersonCamera::Yaw() const noexcept
    {
        return
            yaw_;
    }

    float ThirdPersonCamera::ControlYaw() const noexcept
    {
        return
            controlYaw_;
    }

    const graphics::CameraView&
        ThirdPersonCamera::View() const noexcept
    {
        return
            view_;
    }
}