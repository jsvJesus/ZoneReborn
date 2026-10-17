
#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace world_editor::terrain {

    struct WorldDocument {
        Settings settings;
        Heightfield terrain;
    };

    struct WorldChunk {
        std::uint32_t startX = 0;
        std::uint32_t startZ = 0;
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        std::vector<float> heights;
    };

    void save_world(
        const Heightfield& terrain,
        const Settings& settings,
        const std::filesystem::path& directory
    );

    [[nodiscard]] WorldDocument load_world(
        const std::filesystem::path& directory
    );

    [[nodiscard]] Settings read_world_settings(
        const std::filesystem::path& directory
    );

    [[nodiscard]] WorldChunk read_world_chunk(
        const std::filesystem::path& directory,
        const Settings& settings,
        std::uint32_t chunkX,
        std::uint32_t chunkZ
    );

}
