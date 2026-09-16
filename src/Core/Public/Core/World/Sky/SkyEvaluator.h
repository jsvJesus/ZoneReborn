#pragma once

#include "Core/Math/Vector3.h"
#include "Core/World/Sky/SkyDefinition.h"

namespace core::world::sky
{
    struct SkySample final
    {
        float timeHours =
            0.0f;

        float daylight =
            0.0f;

        math::Vector3
            lightColour;

        math::Vector3
            ambientColour;

        math::Vector3
            sunDirection;
    };

    class SkyEvaluator final
    {
    public:
        [[nodiscard]]
        static SkySample Evaluate(
            const SkyDefinition& definition,
            float elapsedSeconds) noexcept;
    };
}