#pragma once

#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

#include <filesystem>

namespace world_editor::terrain {

    [[nodiscard]] Heightfield correct_for_layout(
        const Heightfield& source,
        world_generator::MapLayout& layout,
        const biome::HydrologyLayer* water
    );

    void save_correction_base(
        const Heightfield& base,
        const Heightfield& corrected,
        const std::filesystem::path& path
    );

    [[nodiscard]] Heightfield load_correction_base(
        const Heightfield& corrected,
        const std::filesystem::path& path
    );

}
