#pragma once

#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Settlement/POIPlanner.h"
#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

namespace world_editor::world_generator {

    void validate_configuration(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield
    );

    [[nodiscard]] MapLayout generate_layout(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield,
        const biome::HydrologyLayer* water = nullptr
    );

}
