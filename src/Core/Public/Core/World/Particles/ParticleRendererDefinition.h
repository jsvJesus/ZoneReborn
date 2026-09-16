#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace core::world::particles
{
    enum class ParticleRendererType : std::uint8_t
    {
        None = 0,
        Sprite,
        SpriteBlend,
        Trail,
        Unsupported
    };

    struct ParticleTextureReference final
    {
        std::string sourceReference;
        std::string logicalPath;

        bool animated =
            false;

        bool exists =
            false;
    };

    struct ParticleRendererDefinition final
    {
        ParticleRendererType type =
            ParticleRendererType::None;

        std::string typeName;

        ParticleTextureReference texture;
        ParticleTextureReference normalMap;

        bool viewDependent =
            false;

        bool local =
            false;

        bool useFog =
            true;

        std::int32_t materialFx =
            0;

        std::int32_t frameCount =
            0;

        float frameRate =
            0.0f;

        std::array<float, 3>
            explicitOrientation{};

        std::array<float, 2>
            textureOffset{};

        float width =
            0.0f;

        std::int32_t skip =
            0;

        std::int32_t steps =
            0;
    };
}