#include "Player/PlayerController.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    constexpr float WalkSpeed =
        3.0f;

    constexpr float RunSpeed =
        6.2f;

    constexpr float Gravity =
        -19.62f;

    constexpr float JumpVelocity =
        6.3f;

    constexpr float CapsuleRadius =
        0.34f;

    constexpr float CapsuleHeight =
        1.78f;

    constexpr float StepHeight =
        0.42f;

    constexpr float GroundProbe =
        1.1f;

    constexpr float RotationSpeed =
        14.0f;

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

        return angle;
    }
}

namespace client::player
{
    bool Controller::IsKeyDown(
        const int key) noexcept
    {
        return
            (
                GetAsyncKeyState(
                    key) &
                0x8000
            ) != 0;
    }

    void Controller::Reset(
        const core::math::Vector3& position,
        const float yaw) noexcept
    {
        position_ =
            position;

        yaw_ =
            yaw;

        verticalVelocity_ =
            0.0f;

        grounded_ =
            true;

        moving_ =
            false;

        running_ =
            false;

        jumpWasDown_ =
            false;
    }

    void Controller::Update(
        const HWND window,
        float deltaSeconds,
        const float cameraYaw,
        const world::Collision& collision) noexcept
    {
        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        moving_ =
            false;

        running_ =
            false;

        if (window == nullptr)
        {
            return;
        }

        const bool active =
            GetForegroundWindow() ==
            window;

        float inputX =
            0.0f;

        float inputZ =
            0.0f;

        if (active)
        {
            if (IsKeyDown('W'))
            {
                inputZ +=
                    1.0f;
            }

            if (IsKeyDown('S'))
            {
                inputZ -=
                    1.0f;
            }

            if (IsKeyDown('D'))
            {
                inputX +=
                    1.0f;
            }

            if (IsKeyDown('A'))
            {
                inputX -=
                    1.0f;
            }
        }

        const float inputLength =
            std::sqrt(
                inputX * inputX +
                inputZ * inputZ);

        if (inputLength >
            0.0001f)
        {
            inputX /=
                inputLength;

            inputZ /=
                inputLength;

            const float sinYaw =
                std::sin(
                    cameraYaw);

            const float cosYaw =
                std::cos(
                    cameraYaw);

            const core::math::Vector3 cameraForward
            {
                sinYaw,
                0.0f,
                cosYaw
            };

            const core::math::Vector3 cameraRight
            {
                cosYaw,
                0.0f,
                -sinYaw
            };

            core::math::Vector3 direction
            {
                cameraForward.x * inputZ +
                    cameraRight.x * inputX,

                0.0f,

                cameraForward.z * inputZ +
                    cameraRight.z * inputX
            };

            const float directionLength =
                std::sqrt(
                    direction.x * direction.x +
                    direction.z * direction.z);

            if (directionLength >
                0.0001f)
            {
                direction.x /=
                    directionLength;

                direction.z /=
                    directionLength;

                running_ =
                    active &&
                    IsKeyDown(
                        VK_SHIFT);

                const float speed =
                    running_
                        ? RunSpeed
                        : WalkSpeed;

                const core::math::Vector3 movement
                {
                    direction.x *
                        speed *
                        deltaSeconds,

                    0.0f,

                    direction.z *
                        speed *
                        deltaSeconds
                };

                MoveHorizontal(
                    movement,
                    collision);

                moving_ =
                    true;

                const float targetYaw =
                    std::atan2(
                        direction.x,
                        direction.z);

                const float difference =
                    NormalizeAngle(
                        targetYaw -
                        yaw_);

                const float factor =
                    1.0f -
                    std::exp(
                        -RotationSpeed *
                        deltaSeconds);

                yaw_ =
                    NormalizeAngle(
                        yaw_ +
                        difference *
                        factor);
            }
        }

        const bool jumpDown =
            active &&
            IsKeyDown(
                VK_SPACE);

        if (jumpDown &&
            !jumpWasDown_ &&
            grounded_)
        {
            grounded_ =
                false;

            verticalVelocity_ =
                JumpVelocity;
        }

        jumpWasDown_ =
            jumpDown;

        if (!grounded_)
        {
            verticalVelocity_ +=
                Gravity *
                deltaSeconds;

            position_.y +=
                verticalVelocity_ *
                deltaSeconds;

            float groundHeight =
                0.0f;

            if (verticalVelocity_ <=
                    0.0f &&
                collision.FindGround(
                    position_,
                    0.25f,
                    GroundProbe +
                        std::abs(
                            verticalVelocity_) *
                        deltaSeconds,
                    groundHeight))
            {
                if (position_.y <=
                    groundHeight +
                    0.05f)
                {
                    position_.y =
                        groundHeight;

                    verticalVelocity_ =
                        0.0f;

                    grounded_ =
                        true;
                }
            }
        }
        else
        {
            float groundHeight =
                0.0f;

            if (collision.FindGround(
                    position_,
                    StepHeight,
                    GroundProbe,
                    groundHeight))
            {
                position_.y =
                    groundHeight;
            }
            else
            {
                grounded_ =
                    false;
            }
        }
    }

    void Controller::MoveHorizontal(
        const core::math::Vector3& movement,
        const world::Collision& collision) noexcept
    {
        core::math::Vector3 target =
            position_;

        target.x +=
            movement.x;

        target.z +=
            movement.z;

        float groundHeight =
            0.0f;

        if (grounded_ &&
            collision.FindGround(
                target,
                StepHeight,
                GroundProbe,
                groundHeight))
        {
            target.y =
                groundHeight;
        }

        if (!collision.BlocksCapsule(
                target,
                CapsuleRadius,
                CapsuleHeight))
        {
            position_ =
                target;

            return;
        }

        target =
            position_;

        target.x +=
            movement.x;

        if (grounded_ &&
            collision.FindGround(
                target,
                StepHeight,
                GroundProbe,
                groundHeight))
        {
            target.y =
                groundHeight;
        }

        if (!collision.BlocksCapsule(
                target,
                CapsuleRadius,
                CapsuleHeight))
        {
            position_ =
                target;
        }

        target =
            position_;

        target.z +=
            movement.z;

        if (grounded_ &&
            collision.FindGround(
                target,
                StepHeight,
                GroundProbe,
                groundHeight))
        {
            target.y =
                groundHeight;
        }

        if (!collision.BlocksCapsule(
                target,
                CapsuleRadius,
                CapsuleHeight))
        {
            position_ =
                target;
        }
    }

    const core::math::Vector3&
    Controller::Position() const noexcept
    {
        return position_;
    }

    float Controller::Yaw() const noexcept
    {
        return yaw_;
    }

    bool Controller::IsMoving() const noexcept
    {
        return moving_;
    }

    bool Controller::IsRunning() const noexcept
    {
        return running_;
    }

    bool Controller::IsGrounded() const noexcept
    {
        return grounded_;
    }

    float Controller::VerticalVelocity() const noexcept
    {
        return
            verticalVelocity_;
    }

    core::math::Transform3x4
    Controller::Transform() const noexcept
    {
        core::math::Transform3x4
            transform =
                core::math::Transform3x4::Identity();

        const float cosine =
            std::cos(
                yaw_);

        const float sine =
            std::sin(
                yaw_);

        transform.values[0] =
            cosine;

        transform.values[1] =
            0.0f;

        transform.values[2] =
            -sine;

        transform.values[3] =
            0.0f;

        transform.values[4] =
            1.0f;

        transform.values[5] =
            0.0f;

        transform.values[6] =
            sine;

        transform.values[7] =
            0.0f;

        transform.values[8] =
            cosine;

        transform.values[9] =
            position_.x;

        transform.values[10] =
            position_.y;

        transform.values[11] =
            position_.z;

        return transform;
    }
}