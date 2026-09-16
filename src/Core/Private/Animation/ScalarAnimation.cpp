#include "Core/Animation/ScalarAnimation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace core::animation
{
    float ScalarAnimationEvaluator::Evaluate(
        const ScalarAnimationTrack& animation,
        const float elapsedSeconds) noexcept
    {
        if (animation.frames.empty())
        {
            return 1.0f;
        }

        if (animation.frames.size() == 1)
        {
            return
                std::max(
                    animation.frames.front().value,
                    0.0f);
        }

        const float sourceDuration =
            animation.frames.back().time;

        if (sourceDuration <=
            0.000001f)
        {
            return
                std::max(
                    animation.frames.back().value,
                    0.0f);
        }

        if (animation.timeScale <=
            0.000001f)
        {
            return
                std::max(
                    animation.frames.front().value,
                    0.0f);
        }

        const float cycleDuration =
            animation.duration >
                0.000001f
                ? animation.duration
                : sourceDuration;

        float cycleTime =
            std::fmod(
                elapsedSeconds *
                    animation.timeScale,
                cycleDuration);

        if (cycleTime < 0.0f)
        {
            cycleTime +=
                cycleDuration;
        }

        const float sampleTime =
            cycleTime *
            (
                sourceDuration /
                cycleDuration
            );

        if (sampleTime <=
            animation.frames.front().time)
        {
            return
                std::max(
                    animation.frames.front().value,
                    0.0f);
        }

        for (std::size_t index = 1;
             index < animation.frames.size();
             ++index)
        {
            const ScalarAnimationFrame& previous =
                animation.frames[
                    index - 1];

            const ScalarAnimationFrame& current =
                animation.frames[
                    index];

            if (sampleTime >
                current.time)
            {
                continue;
            }

            const float frameDuration =
                current.time -
                previous.time;

            if (frameDuration <=
                0.000001f)
            {
                return
                    std::max(
                        current.value,
                        0.0f);
            }

            const float factor =
                std::clamp(
                    (
                        sampleTime -
                        previous.time
                    ) /
                    frameDuration,
                    0.0f,
                    1.0f);

            const float value =
                previous.value +
                (
                    current.value -
                    previous.value
                ) *
                factor;

            return
                std::max(
                    value,
                    0.0f);
        }

        return
            std::max(
                animation.frames.back().value,
                0.0f);
    }
}