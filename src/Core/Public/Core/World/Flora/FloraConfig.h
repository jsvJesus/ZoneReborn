#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace core::world::flora
{
    struct FloraTextureRule final
    {
        std::string textureReference;

        float weight =
            1.0f;
    };

    struct FloraVisualRule final
    {
        std::string visualReference;

        float density =
            1.0f;

        float scaleVariation =
            0.0f;

        float flex =
            1.0f;
    };

    struct FloraGeneratorRule final
    {
        float frequency =
            1.0f;

        std::vector<FloraVisualRule>
            visuals;
    };

    struct FloraEcotype final
    {
        std::string name;

        std::string soundTag;

        std::vector<FloraTextureRule>
            textures;

        std::vector<FloraGeneratorRule>
            generators;
    };

    struct FloraConfig final
    {
        std::size_t vertexBufferSize =
            0;

        std::uint32_t textureWidth =
            0;

        std::uint32_t textureHeight =
            0;

        std::uint32_t alphaTestReference =
            0;

        std::uint32_t shadowAlphaTestReference =
            0;

        float alphaTestDistance =
            0.0f;

        float alphaBlendDistance =
            0.0f;

        float alphaTestFadePercent =
            0.0f;

        float alphaBlendFadePercent =
            0.0f;

        std::vector<FloraEcotype>
            ecotypes;

        [[nodiscard]]
        std::vector<const FloraEcotype*>
        FindEcotypesByTexture(
            std::string_view textureReference) const;
    };
}