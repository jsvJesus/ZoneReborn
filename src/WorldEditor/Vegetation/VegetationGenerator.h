#pragma once

#include "WorldEditor/Biome/BiomeGenerator.h"
#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace world_editor::vegetation {

    enum class VegetationKind : std::uint8_t {
        Grass,
        Bush,
        Tree
    };

    enum class VegetationAsset : std::uint8_t {
        FieldGrass,
        DryGrass,
        SwampGrass,
        GreenBush,
        MarshReeds,
        FirTree,
        PineTree,
        SwampTree,
        Count
    };

    struct VegetationSettings {
        std::uint32_t seed = 20261018;

        float grassDensity = 0.75f;
        float bushDensity = 0.40f;
        float treeDensity = 0.50f;

        float roadClearanceMeters = 12.0f;
        float poiClearanceMeters = 12.0f;
        float exitClearanceMeters = 20.0f;

        std::uint32_t maxInstances = 160000;
    };

    struct VegetationInstance {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float yawRadians = 0.0f;
        float scale = 1.0f;

        VegetationKind kind = VegetationKind::Grass;
        VegetationAsset asset = VegetationAsset::FieldGrass;
    };

    struct VegetationLayer {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        VegetationSettings settings;

        // Three independent R8 density maps.
        std::vector<std::uint8_t> grassMask;
        std::vector<std::uint8_t> bushMask;
        std::vector<std::uint8_t> treeMask;

        std::vector<VegetationInstance> instances;

        [[nodiscard]] bool valid() const noexcept;
    };

    [[nodiscard]] std::string_view asset_relative_path(
        VegetationAsset asset
    ) noexcept;

    [[nodiscard]] VegetationLayer generate_vegetation(
        const terrain::Heightfield& terrain,
        const biome::BiomeLayer& biomes,
        const biome::HydrologyLayer* hydrology,
        const world_generator::MapLayout* layout,
        const VegetationSettings& settings
    );

}
