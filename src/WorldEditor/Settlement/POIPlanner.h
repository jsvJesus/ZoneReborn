#pragma once

#include "WorldEditor/Settlement/POIPlacement.h"
#include "WorldEditor/WorldGenerator/MapGenerationSettings.h"
#include "WorldEditor/Terrain/TerrainData.h"

#include <functional>
#include <vector>

namespace world_editor::settlement {

    using WaterPredicate = std::function<bool(float, float)>;

    [[nodiscard]] std::vector<POIPlacement> place_pois(
        const world_generator::MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield,
        const WaterPredicate& isWater
    );

}
