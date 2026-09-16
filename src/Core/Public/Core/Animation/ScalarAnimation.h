#pragma once

#include <string>
#include <vector>

namespace core::animation
{
    struct ScalarAnimationFrame final
    {
        float time = 0.0f;
        float value = 1.0f;
    };

    struct ScalarAnimationTrack final
    {
        std::string resource;

        float timeScale = 1.0f;
        float duration = 0.0f;

        std::vector<ScalarAnimationFrame>
            frames;
    };

    class ScalarAnimationEvaluator final
    {
    public:
        [[nodiscard]]
        static float Evaluate(
            const ScalarAnimationTrack& animation,
            float elapsedSeconds) noexcept;
    };
}