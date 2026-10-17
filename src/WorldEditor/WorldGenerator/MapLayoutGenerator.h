#pragma once

#include "WorldEditor/WorldGenerator/MapLayout.h"
#include "WorldEditor/Settlement/POIPlanner.h"
#include "WorldEditor/Terrain/TerrainData.h"

namespace world_editor::world_generator {

    void validate_configuration(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield
    );

    [[nodiscard]] MapLayout generate_layout(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield,
        const settlement::WaterPredicate& isWater = {}
    );

}
