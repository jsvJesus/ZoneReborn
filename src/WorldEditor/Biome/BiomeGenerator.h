#pragma once

#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Terrain/TerrainData.h"

#include <cstdint>
#include <vector>

namespace world_editor::biome {

    enum class BiomeKind : std::uint8_t {
        Grassland,
        Forest,
        Sandy,
        Rocky,
        Wetland
    };

    struct BiomeSettings {
        std::uint32_t seed = 20261018;

        float moisture = 0.55f;
        float forestCoverage = 0.55f;
        float rockSlopeDegrees = 22.0f;
        float textureScaleMeters = 6.0f;

        [[nodiscard]] bool operator==(const BiomeSettings&) const = default;
    };

    struct BiomeLayer {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        BiomeSettings settings;

        std::vector<BiomeKind> zones;

        // RGBA8 material weights:
        // R = Grass
        // G = Soil
        // B = Sand
        // A = Rock
        // Marsh = 1 - R - G - B - A
        std::vector<std::uint8_t> materialWeights;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] BiomeKind at_world(
            float worldX,
            float worldZ
        ) const noexcept;
    };

    [[nodiscard]] BiomeLayer generate_biomes(
        const terrain::Heightfield& heightfield,
        const HydrologyLayer* hydrology,
        const BiomeSettings& settings
    );

}
