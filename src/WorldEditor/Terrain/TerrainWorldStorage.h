
#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <filesystem>

namespace world_editor::terrain {

    struct WorldDocument {
        Settings settings;
        Heightfield terrain;
    };

    void save_world(
        const Heightfield& terrain,
        const Settings& settings,
        const std::filesystem::path& directory
    );

    [[nodiscard]] WorldDocument load_world(
        const std::filesystem::path& directory
    );

}
