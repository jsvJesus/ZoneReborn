#pragma once

#include "Core/Math/Vector3.h"

namespace core::world::particles
{
    class ParticleCollisionQuery
    {
    public:
        virtual ~ParticleCollisionQuery() = default;

        [[nodiscard]]
        virtual bool Raycast(
            const math::Vector3& start,
            const math::Vector3& end,
            float& fraction,
            math::Vector3& normal) const noexcept = 0;
    };
}
