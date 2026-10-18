#pragma once

#include "WorldEditor/Road/RoadGraph.h"
#include "WorldEditor/Terrain/TerrainData.h"

#include <vector>

namespace world_editor::settlement {
    struct POIPlacement;
}

namespace world_editor::road {

    void route_graph(
        RoadGraph& graph,
        const terrain::Heightfield& terrain,
        const biome::HydrologyLayer* water,
        const std::vector<settlement::POIPlacement>& pois
    );

}
