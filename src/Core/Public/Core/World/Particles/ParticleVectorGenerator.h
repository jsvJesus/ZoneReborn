#pragma once

#include "Core/Math/Vector3.h"

#include <cstdint>
#include <string>

namespace core::world::particles
{
    enum class ParticleVectorGeneratorType : std::uint8_t
    {
        None = 0,
        Point,
        Line,
        Cylinder,
        Sphere,
        Box,
        Unsupported
    };

    struct ParticleVectorGenerator final
    {
        ParticleVectorGeneratorType type =
            ParticleVectorGeneratorType::None;

        std::string typeName;
        std::string nameId;

        math::Vector3 position{};
        math::Vector3 origin{};
        math::Vector3 direction{};
        math::Vector3 centre{};

        math::Vector3 corner{};
        math::Vector3 opposite{};

        math::Vector3 basisU{};
        math::Vector3 basisV{};

        float minRadius =
            0.0f;

        float maxRadius =
            0.0f;
    };
}