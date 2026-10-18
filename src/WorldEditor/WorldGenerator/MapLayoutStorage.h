#pragma once

#include "WorldEditor/WorldGenerator/MapLayout.h"
#include "WorldEditor/Terrain/TerrainData.h"

#include <filesystem>

namespace world_editor::world_generator {

    void save_map_layout(
        const MapLayout& layout,
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& path
    );

    [[nodiscard]] MapLayout load_map_layout(
        const std::filesystem::path& path,
        const terrain::Settings& terrainSettings,
        std::uint64_t sourceFingerprint
    );

}
