#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

namespace world_editor::terrain {

    [[nodiscard]] Heightfield generate(
        const Settings& settings
    );

}
