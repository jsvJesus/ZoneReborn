#include "Player/PlayerController.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Gravity =
        -19.62f;

    constexpr float JumpVelocity =
        6.3f;

    constexpr float CapsuleRadius =
        0.34f;

    constexpr float StepHeight =
        0.42f;

    constexpr float GroundProbe =
        1.1f;
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

    MovementDirection Controller::ResolveDirection(
        const float right,
        const float forward) noexcept
    {
        const bool hasForward =
            forward >
            0.0f;

        const bool hasBackward =
            forward <
            0.0f;

        const bool hasRight =
            right >
            0.0f;

        const bool hasLeft =
            right <
            0.0f;

        if (hasForward)
        {
            if (hasRight)
            {
                return
                    MovementDirection::ForwardRight;
            }

            if (hasLeft)
            {
                return
                    MovementDirection::ForwardLeft;
            }

            return
                MovementDirection::Forward;
        }

        if (hasBackward)
        {
            if (hasRight)
            {
                return
                    MovementDirection::BackwardRight;
            }

            if (hasLeft)
            {
                return
                    MovementDirection::BackwardLeft;
            }

            return
                MovementDirection::Backward;
        }

        if (hasRight)
        {
            return
                MovementDirection::Right;
        }

        if (hasLeft)
        {
            return
                MovementDirection::Left;
        }

        return
            MovementDirection::None;
    }

    void Controller::Reset(
        const core::math::Vector3& position,
        const float yaw) noexcept
    {
        position_ =
            position;

        locomotion_.Reset();

        turn_.Reset(
            yaw);

        movementDirection_ =
            MovementDirection::None;

        moveForwardInput_ =
            0.0f;

        moveRightInput_ =
            0.0f;

        verticalVelocity_ =
            0.0f;

        grounded_ =
            true;

        moving_ =
            false;

        sprintWasDown_ =
            false;

        crouchWasDown_ =
            false;

        walkWasDown_ =
            false;

        jumpWasDown_ =
            false;
    }

    void Controller::Update(
        const HWND window,
        float deltaSeconds,
        const float controlYaw,
        const world::Collision& collision) noexcept
    {
        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        const bool active =
            window !=
                nullptr &&
            GetForegroundWindow() ==
                window;

        const bool sprintDown =
            active &&
            IsKeyDown(
                VK_SHIFT);

        const bool crouchDown =
            active &&
            IsKeyDown(
                VK_CONTROL);

        const bool walkDown =
            active &&
            IsKeyDown(
                'X');

        if (sprintDown &&
            !sprintWasDown_)
        {
            locomotion_.ToggleSprint();
        }

        if (walkDown &&
            !walkWasDown_)
        {
            locomotion_.ToggleWalk();
        }

        if (crouchDown &&
            !crouchWasDown_)
        {
            if (!locomotion_.IsCrouched())
            {
                locomotion_.SetCrouched(
                    true);
            }
            else
            {
                const bool blocked =
                    collision.BlocksCapsule(
                        position_,
                        CapsuleRadius,
                        locomotion_.
                            StandingCapsuleHeight());

                if (!blocked)
                {
                    locomotion_.SetCrouched(
                        false);
                }
            }
        }

        sprintWasDown_ =
            sprintDown;

        crouchWasDown_ =
            crouchDown;

        walkWasDown_ =
            walkDown;

        float inputRight =
            0.0f;

        float inputForward =
            0.0f;

        if (active)
        {
            if (IsKeyDown(
                    'W') ||
                IsKeyDown(
                    VK_UP))
            {
                inputForward +=
                    1.0f;
            }

            if (IsKeyDown(
                    'S') ||
                IsKeyDown(
                    VK_DOWN))
            {
                inputForward -=
                    1.0f;
            }

            if (IsKeyDown(
                    'D') ||
                IsKeyDown(
                    VK_RIGHT))
            {
                inputRight +=
                    1.0f;
            }

            if (IsKeyDown(
                    'A') ||
                IsKeyDown(
                    VK_LEFT))
            {
                inputRight -=
                    1.0f;
            }
        }

        movementDirection_ =
            ResolveDirection(
                inputRight,
                inputForward);

        const float inputLength =
            std::sqrt(
                inputRight *
                    inputRight +
                inputForward *
                    inputForward);

        moving_ =
            inputLength >
            0.0001f;

        moveRightInput_ =
            0.0f;

        moveForwardInput_ =
            0.0f;

        if (moving_)
        {
            moveRightInput_ =
                inputRight /
                inputLength;

            moveForwardInput_ =
                inputForward /
                inputLength;
        }

        //
        // В SO control yaw определяется мышью.
        // WASD НЕ разворачивает персонажа.
        //
        turn_.Update(
            controlYaw,
            deltaSeconds,
            moving_,
            locomotion_.IsCrouched(),
            grounded_);

        if (moving_)
        {
            const float sinYaw =
                std::sin(
                    turn_.ControlYaw());

            const float cosYaw =
                std::cos(
                    turn_.ControlYaw());

            const core::math::Vector3 forward
            {
                sinYaw,
                0.0f,
                cosYaw
            };

            const core::math::Vector3 right
            {
                cosYaw,
                0.0f,
                -sinYaw
            };

            core::math::Vector3 direction
            {
                forward.x *
                    moveForwardInput_ +
                right.x *
                    moveRightInput_,

                0.0f,

                forward.z *
                    moveForwardInput_ +
                right.z *
                    moveRightInput_
            };

            const float directionLength =
                std::sqrt(
                    direction.x *
                        direction.x +
                    direction.z *
                        direction.z);

            if (directionLength >
                0.0001f)
            {
                direction.x /=
                    directionLength;

                direction.z /=
                    directionLength;
            }

            const float speed =
                locomotion_.Speed(
                    movementDirection_);

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
        }

        const bool jumpDown =
            active &&
            IsKeyDown(
                VK_SPACE);

        if (jumpDown &&
            !jumpWasDown_ &&
            grounded_ &&
            !locomotion_.IsCrouched())
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
        const float capsuleHeight =
            locomotion_.CapsuleHeight();

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
                capsuleHeight))
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
                capsuleHeight))
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
                capsuleHeight))
        {
            position_ =
                target;
        }
    }

    void Controller::CompleteTurn() noexcept
    {
        turn_.CompleteTurn();
    }

    const core::math::Vector3&
    Controller::Position() const noexcept
    {
        return
            position_;
    }

    float Controller::Yaw() const noexcept
    {
        return
            turn_.ControlYaw();
    }

    float Controller::ModelYaw() const noexcept
    {
        return
            turn_.ModelYaw();
    }

    float Controller::BodyYawOffset() const noexcept
    {
        return
            turn_.BodyYawOffset();
    }

    float Controller::FootTwistYaw() const noexcept
    {
        return
            turn_.FootTwistYaw();
    }

    int Controller::TurnDirectionSign() const noexcept
    {
        return
            turn_.DirectionSign();
    }

    float Controller::MoveForwardInput() const noexcept
    {
        return
            moveForwardInput_;
    }

    float Controller::MoveRightInput() const noexcept
    {
        return
            moveRightInput_;
    }

    MovementDirection Controller::Direction() const noexcept
    {
        return
            movementDirection_;
    }

    bool Controller::IsMoving() const noexcept
    {
        return
            moving_;
    }

    LocomotionMode
    Controller::SelectedLocomotionMode() const noexcept
    {
        return
            locomotion_.Mode();
    }

    bool Controller::IsWalking() const noexcept
    {
        return
            locomotion_.EffectiveMode(
                movementDirection_) ==
            LocomotionMode::Walk;
    }

    bool Controller::IsRunning() const noexcept
    {
        return
            locomotion_.EffectiveMode(
                movementDirection_) ==
            LocomotionMode::Run;
    }

    bool Controller::IsSprinting() const noexcept
    {
        return
            !locomotion_.IsCrouched() &&
            locomotion_.EffectiveMode(
                movementDirection_) ==
                LocomotionMode::Sprint;
    }

    bool Controller::IsCrouched() const noexcept
    {
        return
            locomotion_.IsCrouched();
    }

    bool Controller::IsGrounded() const noexcept
    {
        return
            grounded_;
    }

    float Controller::VerticalVelocity() const noexcept
    {
        return
            verticalVelocity_;
    }

    float Controller::CameraTargetHeight() const noexcept
    {
        return
            locomotion_.CameraTargetHeight();
    }

    core::math::Transform3x4
    Controller::Transform() const noexcept
    {
        core::math::Transform3x4 transform =
            core::math::Transform3x4::Identity();

        const float yaw =
            turn_.ModelYaw();

        const float cosine =
            std::cos(
                yaw);

        const float sine =
            std::sin(
                yaw);

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

        return
            transform;
    }
}