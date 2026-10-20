#pragma once

#include "WorldEditor/Road/RoadGraph.h"

#include <cstddef>
#include <vector>

namespace world_editor::road {

    enum class CrossingKind {
        WoodBridge,
        MetalBridge
    };

    struct RoadCrossing {
        std::size_t edgeIndex = 0;
        std::size_t firstPoint = 0;
        std::size_t lastPoint = 0;
        biome::WaterKind water = biome::WaterKind::River;
        float lengthMeters = 0.0f;
        CrossingKind kind = CrossingKind::WoodBridge;
    };

    [[nodiscard]] float crossing_deck_width(CrossingKind kind) noexcept;

    [[nodiscard]] std::vector<RoadCrossing> find_crossings(
        const RoadGraph& graph
    );

}
