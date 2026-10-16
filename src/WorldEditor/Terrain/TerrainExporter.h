#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <filesystem>

namespace world_editor::terrain {

    void save_obj(
        const Heightfield& terrain,
        const std::filesystem::path& filePath
    );

    void save_pgm(
        const Heightfield& terrain,
        const std::filesystem::path& filePath
    );

}
