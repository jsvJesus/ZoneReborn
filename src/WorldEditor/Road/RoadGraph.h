#pragma once

#include <cstdint>
#include <vector>

namespace world_editor::settlement {
    struct POIPlacement;
}

namespace world_editor::world_generator {
    struct MapExit;
}

namespace world_editor::road {

    enum class RoadNodeKind : std::uint32_t {
        POI,
        Exit
    };

    struct RoadNode {
        RoadNodeKind kind = RoadNodeKind::POI;
        std::uint32_t referenceId = 0;
        float x = 0.0f;
        float z = 0.0f;
    };

    struct RoadEdge {
        std::uint32_t from = 0;
        std::uint32_t to = 0;
    };

    struct RoadGraph {
        std::vector<RoadNode> nodes;
        std::vector<RoadEdge> edges;
    };

    [[nodiscard]] RoadGraph build_graph(
        const std::vector<settlement::POIPlacement>& pois,
        const std::vector<world_generator::MapExit>& exits
    );

    [[nodiscard]] bool is_connected(const RoadGraph& graph);

}
