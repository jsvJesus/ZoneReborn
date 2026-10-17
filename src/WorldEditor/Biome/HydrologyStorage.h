#pragma once

#include "WorldEditor/Biome/Hydrology.h"

#include <filesystem>

namespace world_editor::biome {

    void save_hydrology(
        const HydrologyLayer& water,
        const terrain::Heightfield& heightfield,
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& path
    );

    [[nodiscard]] HydrologyLayer load_hydrology(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& path
    );

}
