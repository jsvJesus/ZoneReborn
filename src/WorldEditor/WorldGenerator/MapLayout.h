#pragma once

#include "WorldEditor/WorldGenerator/MapGenerationSettings.h"
#include "WorldEditor/Settlement/POIPlacement.h"
#include "WorldEditor/Road/RoadGraph.h"

#include <cstdint>
#include <vector>

namespace world_editor::world_generator {

    enum class BoundarySide : std::uint32_t {
        North,
        East,
        South,
        West
    };

    struct MapExit {
        std::uint32_t id = 0;
        BoundarySide side = BoundarySide::North;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct MapLayout {
        static constexpr std::uint32_t currentVersion = 1;

        std::uint32_t version = currentVersion;
        MapGenerationSettings settings;
        std::vector<settlement::POIPlacement> pois;
        std::vector<MapExit> exits;
        road::RoadGraph roads;
    };

}
