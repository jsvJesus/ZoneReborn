#pragma once

#include "WorldEditor/Biome/BiomeGenerator.h"
#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

#include <cstdint>
#include <string>
#include <vector>

namespace world_editor::vegetation {

    struct VegetationAssetCatalog;

    enum class VegetationKind : std::uint8_t {
        Grass,
        Bush,
        Tree,
        Unclassified
    };

    struct VegetationSettings {
        std::uint32_t seed = 20261018;
        std::uint32_t decorationSeed = 20261021;

        float grassDensity = 0.75f;
        float bushDensity = 0.40f;
        float treeDensity = 0.50f;

        float roadClearanceMeters = 12.0f;
        float poiClearanceMeters = 12.0f;
        float exitClearanceMeters = 20.0f;

        std::uint32_t maxInstances = 160000;

        [[nodiscard]] bool operator==(const VegetationSettings&) const = default;
    };

    struct VegetationInstance {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float yawRadians = 0.0f;
        float scale = 1.0f;

        VegetationKind kind = VegetationKind::Grass;

        // Index in VegetationLayer::assetPaths.
        std::uint32_t assetIndex = 0;
    };

    struct VegetationLayer {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        VegetationSettings settings;

        std::vector<std::uint8_t> grassMask;
        std::vector<std::uint8_t> bushMask;
        std::vector<std::uint8_t> treeMask;

        // Stable relative paths for generated instances.
        std::vector<std::string> assetPaths;

        std::vector<VegetationInstance> instances;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] const std::string& asset_path(
            const VegetationInstance& instance
        ) const;
    };

    [[nodiscard]] VegetationLayer generate_vegetation(
        const terrain::Heightfield& terrain,
        const biome::BiomeLayer& biomes,
        const biome::HydrologyLayer* hydrology,
        const world_generator::MapLayout* layout,
        const VegetationSettings& settings,
        const VegetationAssetCatalog& catalog
    );

}
