#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <cstdint>
#include <vector>

namespace world_editor::biome {

    enum class WaterKind : std::uint8_t {
        Dry,
        River,
        Lake,
        Marsh
    };

    struct WaterPoint {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct LakeArea {
        WaterPoint center;
        float radiusMeters = 0.0f;
    };

    struct HydrologySettings {
        std::uint32_t seed = 20261017;

        bool riverEnabled = true;
        std::uint32_t lakeCount = 2;

        float riverWidthMeters = 32.0f;
        float marshWidthMeters = 32.0f;
    };

    struct HydrologyLayer {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        HydrologySettings settings;

        std::vector<WaterKind> cells;
        std::vector<WaterPoint> river;
        std::vector<LakeArea> lakes;
        std::vector<WaterPoint> marshMarkers;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] WaterKind at_world(
            float x,
            float z
        ) const noexcept;

        [[nodiscard]] bool blocks_poi(
            float x,
            float z
        ) const noexcept;
    };

    [[nodiscard]] HydrologyLayer generate_hydrology(
        const terrain::Heightfield& terrain,
        const HydrologySettings& settings
    );

}
