#include "Core/World/Sky/SkyEvaluator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    float NormalizeTime(
        float value) noexcept
    {
        value =
            std::fmod(
                value,
                24.0f);

        if (value < 0.0f)
        {
            value +=
                24.0f;
        }

        return value;
    }

    core::math::Vector3 LerpColour(
        const core::math::Vector3& first,
        const core::math::Vector3& second,
        const float factor) noexcept
    {
        return
        {
            first.x +
                (
                    second.x -
                    first.x
                ) *
                factor,

            first.y +
                (
                    second.y -
                    first.y
                ) *
                factor,

            first.z +
                (
                    second.z -
                    first.z
                ) *
                factor
        };
    }

    core::math::Vector3 SampleKeys(
        const std::vector<
            core::world::sky::SkyColourKey>& keys,
        const float time) noexcept
    {
        if (keys.empty())
        {
            return {};
        }

        if (keys.size() == 1)
        {
            return
                keys.front().colour;
        }

        constexpr float Epsilon =
            0.00001f;

        for (const core::world::sky::SkyColourKey& key :
             keys)
        {
            if (std::abs(
                    key.time -
                    time) <=
                Epsilon)
            {
                return
                    key.colour;
            }
        }

        std::size_t nextIndex =
            0;

        while (nextIndex <
                   keys.size() &&
               keys[nextIndex].time <
                   time)
        {
            ++nextIndex;
        }

        const core::world::sky::SkyColourKey*
            previous =
                nullptr;

        const core::world::sky::SkyColourKey*
            next =
                nullptr;

        float previousTime =
            0.0f;

        float nextTime =
            0.0f;

        float sampleTime =
            time;

        if (nextIndex == 0)
        {
            previous =
                &keys.back();

            next =
                &keys.front();

            previousTime =
                previous->time -
                24.0f;

            nextTime =
                next->time;
        }
        else if (nextIndex >=
                 keys.size())
        {
            previous =
                &keys.back();

            next =
                &keys.front();

            previousTime =
                previous->time;

            nextTime =
                next->time +
                24.0f;
        }
        else
        {
            previous =
                &keys[
                    nextIndex -
                    1];

            next =
                &keys[
                    nextIndex];

            previousTime =
                previous->time;

            nextTime =
                next->time;
        }

        const float duration =
            nextTime -
            previousTime;

        if (duration <=
            Epsilon)
        {
            return
                next->colour;
        }

        const float factor =
            std::clamp(
                (
                    sampleTime -
                    previousTime
                ) /
                duration,
                0.0f,
                1.0f);

        return
            LerpColour(
                previous->colour,
                next->colour,
                factor);
    }
}

namespace core::world::sky
{
    SkySample SkyEvaluator::Evaluate(
        const SkyDefinition& definition,
        const float elapsedSeconds) noexcept
    {
        SkySample
            sample;

        const float hoursPassed =
            definition.hourLengthSeconds >
                0.0001f
                ? elapsedSeconds /
                    definition.hourLengthSeconds
                : 0.0f;

        sample.timeHours =
            NormalizeTime(
                definition.startTimeHours +
                hoursPassed);

        sample.lightColour =
            SampleKeys(
                definition.lightKeys,
                sample.timeHours);

        sample.ambientColour =
            SampleKeys(
                definition.ambientKeys,
                sample.timeHours);

        const float solarAngle =
            (
                sample.timeHours -
                6.0f
            ) /
            24.0f *
            Pi *
            2.0f;

        const float maximumElevation =
            definition.sunAngleDegrees *
            Pi /
            180.0f;

        const float elevation =
            std::sin(
                solarAngle) *
            maximumElevation;

        const float horizontal =
            std::cos(
                elevation);

        sample.sunDirection.x =
            std::cos(
                solarAngle) *
            horizontal;

        sample.sunDirection.y =
            std::sin(
                elevation);

        sample.sunDirection.z =
            std::sin(
                solarAngle) *
            horizontal;

        sample.daylight =
            std::clamp(
                (
                    sample.sunDirection.y +
                    0.08f
                ) /
                0.18f,
                0.0f,
                1.0f);

        return sample;
    }
}