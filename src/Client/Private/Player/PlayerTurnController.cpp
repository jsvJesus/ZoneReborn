#include "Player/PlayerTurnController.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    constexpr float TwoPi =
        Pi *
        2.0f;

    constexpr float StandingTurnStart =
        90.0f *
        Pi /
        180.0f;

    constexpr float StandingTurnMaximum =
        100.0f *
        Pi /
        180.0f;

    constexpr float CrouchedTurnStart =
        65.0f *
        Pi /
        180.0f;

    constexpr float CrouchedTurnMaximum =
        80.0f *
        Pi /
        180.0f;

    constexpr float TurnAnimationAngle =
        90.0f *
        Pi /
        180.0f;

    //
    // SO:
    // MODEL_SMOOTH.STAY_BODY_TWIST = 15.0
    //
    constexpr float MovingYawResponse =
        15.0f;

    //
    // SO:
    // MODEL_SMOOTH.STAY_FOOT_TWIST = 19.0
    //
    constexpr float FootTwistResponse =
        19.0f;

    //
    // SO tracker_global.pyson:
    // foot_rotate_limit = 0.45
    //
    constexpr float FootRotateLimit =
        0.45f;

    float NormalizeAngle(
        float angle) noexcept
    {
        while (angle >
               Pi)
        {
            angle -=
                TwoPi;
        }

        while (angle <
               -Pi)
        {
            angle +=
                TwoPi;
        }

        return
            angle;
    }

    float ResponseFactor(
        const float response,
        const float deltaSeconds) noexcept
    {
        return
            1.0f -
            std::exp(
                -response *
                deltaSeconds);
    }
}

namespace client::player
{
    void TurnController::Reset(
        const float yaw) noexcept
    {
        controlYaw_ =
            NormalizeAngle(
                yaw);

        modelYaw_ =
            controlYaw_;

        bodyYawOffset_ =
            0.0f;

        footTwistYaw_ =
            0.0f;

        direction_ =
            TurnDirection::None;

        crouched_ =
            false;
    }

    void TurnController::Update(
        const float controlYaw,
        float deltaSeconds,
        const bool moving,
        const bool crouched,
        const bool grounded) noexcept
    {
        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        controlYaw_ =
            NormalizeAngle(
                controlYaw);

        crouched_ =
            crouched;

        if (!grounded)
        {
            direction_ =
                TurnDirection::None;

            UpdateTwist(
                deltaSeconds);

            return;
        }

        if (moving)
        {
            direction_ =
                TurnDirection::None;

            const float difference =
                NormalizeAngle(
                    controlYaw_ -
                    modelYaw_);

            const float factor =
                ResponseFactor(
                    MovingYawResponse,
                    deltaSeconds);

            modelYaw_ =
                NormalizeAngle(
                    modelYaw_ +
                    difference *
                    factor);

            UpdateTwist(
                deltaSeconds);

            return;
        }

        const float difference =
            NormalizeAngle(
                controlYaw_ -
                modelYaw_);

        const float startAngle =
            crouched_
                ? CrouchedTurnStart
                : StandingTurnStart;

        if (direction_ ==
                TurnDirection::None &&
            std::abs(
                difference) >=
                startAngle)
        {
            direction_ =
                difference >
                    0.0f
                    ? TurnDirection::Right
                    : TurnDirection::Left;
        }

        UpdateTwist(
            deltaSeconds);
    }

    void TurnController::CompleteTurn() noexcept
    {
        if (direction_ ==
            TurnDirection::None)
        {
            return;
        }

        const float difference =
            NormalizeAngle(
                controlYaw_ -
                modelYaw_);

        if (std::abs(
                difference) <=
            0.001f)
        {
            modelYaw_ =
                controlYaw_;

            direction_ =
                TurnDirection::None;

            UpdateTwist(
                0.0f);

            return;
        }

        const float amount =
            std::min(
                std::abs(
                    difference),
                TurnAnimationAngle);

        modelYaw_ =
            NormalizeAngle(
                modelYaw_ +
                std::copysign(
                    amount,
                    difference));

        direction_ =
            TurnDirection::None;

        UpdateTwist(
            0.0f);
    }

    void TurnController::UpdateTwist(
        const float deltaSeconds) noexcept
    {
        const float difference =
            NormalizeAngle(
                controlYaw_ -
                modelYaw_);

        const float maximumBodyYaw =
            crouched_
                ? CrouchedTurnMaximum
                : StandingTurnMaximum;

        bodyYawOffset_ =
            std::clamp(
                difference,
                -maximumBodyYaw,
                maximumBodyYaw);

        const float targetFootTwist =
            std::clamp(
                bodyYawOffset_,
                -FootRotateLimit,
                FootRotateLimit);

        if (deltaSeconds <=
            0.0f)
        {
            return;
        }

        const float factor =
            ResponseFactor(
                FootTwistResponse,
                deltaSeconds);

        footTwistYaw_ +=
            (
                targetFootTwist -
                footTwistYaw_
            ) *
            factor;
    }

    float TurnController::ControlYaw() const noexcept
    {
        return
            controlYaw_;
    }

    float TurnController::ModelYaw() const noexcept
    {
        return
            modelYaw_;
    }

    float TurnController::BodyYawOffset() const noexcept
    {
        return
            bodyYawOffset_;
    }

    float TurnController::FootTwistYaw() const noexcept
    {
        return
            footTwistYaw_;
    }

    TurnDirection
    TurnController::Direction() const noexcept
    {
        return
            direction_;
    }

    int TurnController::DirectionSign() const noexcept
    {
        switch (direction_)
        {
        case TurnDirection::Left:
            return
                -1;

        case TurnDirection::Right:
            return
                1;

        case TurnDirection::None:
            break;
        }

        return
            0;
    }
}