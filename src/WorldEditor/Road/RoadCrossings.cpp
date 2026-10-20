#include "WorldEditor/Road/RoadCrossings.h"

namespace world_editor::road {

    namespace {

        constexpr float woodMaxSpanMeters = 26.0f;

        [[nodiscard]] bool deep_water(
            const biome::WaterKind kind
        ) noexcept {
            return kind == biome::WaterKind::River ||
                kind == biome::WaterKind::Lake;
        }

    }

    float crossing_deck_width(CrossingKind kind) noexcept {
        return kind == CrossingKind::MetalBridge ? 2.58f : 5.66f;
    }

    std::vector<RoadCrossing> find_crossings(
        const RoadGraph& graph
    ) {
        std::vector<RoadCrossing> result;

        for (std::size_t edgeIndex = 0;
             edgeIndex < graph.edges.size();
             ++edgeIndex) {
            const auto& route = graph.edges[edgeIndex].route;

            for (std::size_t point = 0; point < route.size();) {
                if (!deep_water(route[point].water)) {
                    ++point;
                    continue;
                }

                const std::size_t firstWet = point;
                bool crossesLake = false;

                while (point < route.size() &&
                    deep_water(route[point].water)) {
                    crossesLake |= route[point].water ==
                        biome::WaterKind::Lake;
                    ++point;
                }

                if (firstWet == 0 || point == route.size()) {
                    continue;
                }

                const std::size_t first = firstWet - 1;
                const std::size_t last = point;
                const float length = route[last].distanceMeters -
                    route[first].distanceMeters;

                if (length <= 0.0f) {
                    continue;
                }

                result.push_back({
                    edgeIndex,
                    first,
                    last,
                    crossesLake
                        ? biome::WaterKind::Lake
                        : biome::WaterKind::River,
                    length,
                    crossesLake || length > woodMaxSpanMeters
                        ? CrossingKind::MetalBridge
                        : CrossingKind::WoodBridge
                });
            }
        }

        return result;
    }

}
