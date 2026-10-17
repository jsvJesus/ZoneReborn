#include "WorldEditor/Road/RoadGraph.h"

#include "WorldEditor/Settlement/POIPlacement.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

#include <cstddef>
#include <limits>
#include <queue>
#include <stdexcept>

namespace world_editor::road {

    namespace {

        [[nodiscard]] double distance_squared(
            const RoadNode& a,
            const RoadNode& b
        ) noexcept {
            const double dx =
                static_cast<double>(a.x) - b.x;

            const double dz =
                static_cast<double>(a.z) - b.z;

            return dx * dx + dz * dz;
        }

    }

    RoadGraph build_graph(
        const std::vector<settlement::POIPlacement>& pois,
        const std::vector<world_generator::MapExit>& exits
    ) {
        RoadGraph graph;

        graph.nodes.reserve(pois.size() + exits.size());

        for (const auto& poi : pois) {
            graph.nodes.push_back({
                RoadNodeKind::POI,
                poi.id,
                poi.centerX,
                poi.centerZ
            });
        }

        for (const auto& exit : exits) {
            graph.nodes.push_back({
                RoadNodeKind::Exit,
                exit.id,
                exit.x,
                exit.z
            });
        }

        const std::size_t count = graph.nodes.size();

        if (count == 0) {
            throw std::invalid_argument(
                "Road graph must have nodes"
            );
        }

        std::vector<bool> visited(count, false);
        visited[0] = true;

        for (std::size_t connected = 1;
             connected < count;
             ++connected) {

            double bestDistance =
                std::numeric_limits<double>::infinity();

            std::size_t bestFrom = count;
            std::size_t bestTo = count;

            for (std::size_t from = 0;
                 from < count;
                 ++from) {

                if (!visited[from]) {
                    continue;
                }

                for (std::size_t to = 0;
                     to < count;
                     ++to) {

                    if (visited[to]) {
                        continue;
                    }

                    const double candidate = distance_squared(
                        graph.nodes[from],
                        graph.nodes[to]
                    );

                    if (candidate < bestDistance) {
                        bestDistance = candidate;
                        bestFrom = from;
                        bestTo = to;
                    }
                }
            }

            if (bestTo == count) {
                throw std::runtime_error(
                    "Cannot connect road graph"
                );
            }

            visited[bestTo] = true;

            graph.edges.push_back({
                static_cast<std::uint32_t>(bestFrom),
                static_cast<std::uint32_t>(bestTo)
            });
        }

        return graph;
    }

    bool is_connected(const RoadGraph& graph) {
        const std::size_t count = graph.nodes.size();

        if (count == 0) {
            return false;
        }

        std::vector<std::vector<std::size_t>> adjacency(
            count
        );

        for (const RoadEdge& edge : graph.edges) {
            if (edge.from >= count ||
                edge.to >= count ||
                edge.from == edge.to) {
                return false;
            }

            adjacency[edge.from].push_back(edge.to);
            adjacency[edge.to].push_back(edge.from);
        }

        std::vector<bool> reached(count, false);
        std::queue<std::size_t> pending;

        reached[0] = true;
        pending.push(0);

        std::size_t visited = 0;

        while (!pending.empty()) {
            const std::size_t node = pending.front();
            pending.pop();
            ++visited;

            for (const std::size_t neighbor : adjacency[node]) {
                if (!reached[neighbor]) {
                    reached[neighbor] = true;
                    pending.push(neighbor);
                }
            }
        }

        return visited == count;
    }

}
