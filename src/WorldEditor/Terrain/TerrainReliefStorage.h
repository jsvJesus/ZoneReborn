#pragma once

#include "WorldEditor/Terrain/TerrainRelief.h"

#include <filesystem>

namespace world_editor::terrain {

    void save_relief_profile(
        const ReliefProfile& profile,
        const Heightfield& heightfield,
        const std::filesystem::path& path
    );

    [[nodiscard]] ReliefProfile load_relief_profile(
        const Heightfield& heightfield,
        const std::filesystem::path& path
    );

}
