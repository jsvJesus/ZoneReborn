#pragma once

#include "Core/Math/BoundingBox.h"
#include "Core/Math/Vector3.h"

#include "Core/World/Particles/ParticleActionDefinition.h"
#include "Core/World/Particles/ParticleRendererDefinition.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::world::particles
{
    struct ParticleLoadStatistics final
    {
        std::size_t systemCount =
            0;

        std::size_t actionCount =
            0;

        std::size_t rendererCount =
            0;

        std::size_t vectorGeneratorCount =
            0;

        std::size_t textureReferenceCount =
            0;

        std::size_t animatedTextureReferenceCount =
            0;

        std::size_t missingTextureCount =
            0;

        std::size_t unsupportedActionCount =
            0;

        std::size_t unsupportedRendererCount =
            0;

        std::size_t unsupportedVectorGeneratorCount =
            0;
    };

    struct ParticleSystemDefinition final
    {
        std::string name;

        std::int32_t serialiseVersion =
            0;

        float windFactor =
            0.0f;

        bool windEnabled =
            false;

        bool explicitTransform =
            false;

        math::Vector3 explicitPosition{};

        math::Vector3 explicitDirection
        {
            0.0f,
            0.0f,
            1.0f
        };

        math::Vector3 localOffset{};

        float maxLod =
            0.0f;

        float fixedFrameRate =
            -1.0f;

        std::int32_t capacity =
            0;

        math::BoundingBox
            boundingBox;

        bool hasBoundingBox =
            false;

        std::vector<ParticleActionDefinition>
            actions;

        ParticleRendererDefinition
            renderer;

        bool hasRenderer =
            false;
    };

    struct ParticleDefinition final
    {
        std::string logicalPath;

        std::vector<ParticleSystemDefinition>
            systems;

        ParticleLoadStatistics
            statistics;

        std::vector<std::string>
            missingTextures;
    };
}