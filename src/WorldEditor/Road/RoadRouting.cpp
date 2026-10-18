#include "WorldEditor/Road/RoadRouting.h"

#include "WorldEditor/Settlement/POIPlacement.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <queue>
#include <stdexcept>
#include <vector>

namespace world_editor::road {

    namespace {

        constexpr std::uint32_t noCell =
            std::numeric_limits<std::uint32_t>::max();

        struct Cell {
            float height = 0.0f;
            float terrainGrade = 0.0f;
            biome::WaterKind water = biome::WaterKind::Dry;
            std::uint32_t poi = noCell;
        };

        struct Pending {
            double estimate = 0.0;
            std::uint32_t index = 0;

            [[nodiscard]] bool operator<(const Pending& other) const noexcept {
                if (estimate != other.estimate) {
                    return estimate > other.estimate;
                }

                return index > other.index;
            }
        };

        constexpr std::array<std::array<int, 2>, 8> directions = {{
            {-1, 0}, {1, 0}, {0, -1}, {0, 1},
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
        }};

        [[nodiscard]] std::uint32_t nearest_cell(
            const terrain::Heightfield& terrain,
            const RoadNode& node
        ) noexcept {
            const auto x = static_cast<std::uint32_t>(std::lround(
                std::clamp(
                    (node.x - terrain.originX) / terrain.spacingMeters,
                    0.0f,
                    static_cast<float>(terrain.samplesX - 1)
                )
            ));

            const auto z = static_cast<std::uint32_t>(std::lround(
                std::clamp(
                    (node.z - terrain.originZ) / terrain.spacingMeters,
                    0.0f,
                    static_cast<float>(terrain.samplesZ - 1)
                )
            ));

            return z * terrain.samplesX + x;
        }

        [[nodiscard]] double water_penalty(
            const biome::WaterKind kind
        ) noexcept {
            switch (kind) {
            case biome::WaterKind::River:
                return 30.0;
            case biome::WaterKind::Lake:
                return 60.0;
            case biome::WaterKind::Marsh:
                return 8.0;
            case biome::WaterKind::Dry:
                return 0.0;
            }

            return 0.0;
        }

        [[nodiscard]] bool allowed(
            const Cell& cell,
            const RoadNode& from,
            const RoadNode& to
        ) noexcept {
            return cell.poi == noCell ||
                (from.kind == RoadNodeKind::POI &&
                    cell.poi == from.referenceId) ||
                (to.kind == RoadNodeKind::POI &&
                    cell.poi == to.referenceId);
        }

        [[nodiscard]] std::vector<Cell> make_cells(
            const terrain::Heightfield& terrain,
            const biome::HydrologyLayer* water,
            const std::vector<settlement::POIPlacement>& pois
        ) {
            std::vector<Cell> cells(terrain.heights.size());

            for (std::uint32_t z = 0; z < terrain.samplesZ; ++z) {
                for (std::uint32_t x = 0; x < terrain.samplesX; ++x) {
                    const std::size_t index =
                        static_cast<std::size_t>(z) * terrain.samplesX + x;

                    Cell& cell = cells[index];
                    cell.height = terrain.heights[index];

                    if (!std::isfinite(cell.height)) {
                        throw std::invalid_argument(
                            "Road routing requires finite Terrain heights"
                        );
                    }

                    const float worldX = terrain.originX +
                        static_cast<float>(x) * terrain.spacingMeters;
                    const float worldZ = terrain.originZ +
                        static_cast<float>(z) * terrain.spacingMeters;

                    if (water) {
                        cell.water = water->at_world(worldX, worldZ);
                    }

                    for (const auto& poi : pois) {
                        if (std::abs(worldX - poi.centerX) <
                                poi.widthMeters * 0.5f + terrain.spacingMeters &&
                            std::abs(worldZ - poi.centerZ) <
                                poi.depthMeters * 0.5f + terrain.spacingMeters) {
                            cell.poi = poi.id;
                            break;
                        }
                    }
                }
            }

            for (std::uint32_t z = 0; z < terrain.samplesZ; ++z) {
                for (std::uint32_t x = 0; x < terrain.samplesX; ++x) {
                    const auto left = (std::max)(x, 1u) - 1;
                    const auto right = (std::min)(x + 1, terrain.samplesX - 1);
                    const auto top = (std::max)(z, 1u) - 1;
                    const auto bottom = (std::min)(z + 1, terrain.samplesZ - 1);

                    const float dx = (
                        cells[static_cast<std::size_t>(z) * terrain.samplesX + right].height -
                        cells[static_cast<std::size_t>(z) * terrain.samplesX + left].height
                    ) / ((right - left) * terrain.spacingMeters);

                    const float dz = (
                        cells[static_cast<std::size_t>(bottom) * terrain.samplesX + x].height -
                        cells[static_cast<std::size_t>(top) * terrain.samplesX + x].height
                    ) / ((bottom - top) * terrain.spacingMeters);

                    cells[static_cast<std::size_t>(z) * terrain.samplesX + x]
                        .terrainGrade = std::hypot(dx, dz);
                }
            }

            return cells;
        }

        [[nodiscard]] double heuristic(
            std::uint32_t cell,
            std::uint32_t target,
            const terrain::Heightfield& terrain
        ) noexcept {
            const auto x = static_cast<int>(cell % terrain.samplesX);
            const auto z = static_cast<int>(cell / terrain.samplesX);
            const auto targetX = static_cast<int>(target % terrain.samplesX);
            const auto targetZ = static_cast<int>(target / terrain.samplesX);

            return std::hypot(
                static_cast<double>(targetX - x),
                static_cast<double>(targetZ - z)
            ) * terrain.spacingMeters;
        }

        void append_point(
            std::vector<RoadRoutePoint>& route,
            float x,
            float y,
            float z,
            biome::WaterKind water
        ) {
            if (!route.empty()) {
                const auto& previous = route.back();
                const float distance = std::hypot(
                    x - previous.x,
                    z - previous.z
                );

                if (distance < 0.001f) {
                    return;
                }

                const float grade = std::abs(y - previous.y) / distance;
                route.push_back({
                    x, y, z,
                    previous.distanceMeters + distance,
                    std::atan(grade) *
                        (180.0f / std::numbers::pi_v<float>),
                    water
                });
            } else {
                route.push_back({x, y, z, 0.0f, 0.0f, water});
            }
        }

        [[nodiscard]] std::vector<RoadRoutePoint> find_route(
            const RoadNode& from,
            const RoadNode& to,
            const terrain::Heightfield& terrain,
            const biome::HydrologyLayer* water,
            const std::vector<Cell>& cells
        ) {
            const std::uint32_t start = nearest_cell(terrain, from);
            const std::uint32_t finish = nearest_cell(terrain, to);
            const std::size_t count = cells.size();

            std::vector<double> costs(
                count,
                std::numeric_limits<double>::infinity()
            );
            std::vector<std::uint32_t> parents(count, noCell);
            std::vector<bool> closed(count, false);
            std::priority_queue<Pending> pending;

            costs[start] = 0.0;
            pending.push({heuristic(start, finish, terrain), start});

            while (!pending.empty()) {
                const std::uint32_t current = pending.top().index;
                pending.pop();

                if (closed[current]) {
                    continue;
                }

                if (current == finish) {
                    break;
                }

                closed[current] = true;

                const int x = static_cast<int>(current % terrain.samplesX);
                const int z = static_cast<int>(current / terrain.samplesX);

                for (const auto& direction : directions) {
                    const int nextX = x + direction[0];
                    const int nextZ = z + direction[1];

                    if (nextX < 0 || nextZ < 0 ||
                        nextX >= static_cast<int>(terrain.samplesX) ||
                        nextZ >= static_cast<int>(terrain.samplesZ)) {
                        continue;
                    }

                    const auto next = static_cast<std::uint32_t>(
                        nextZ * static_cast<int>(terrain.samplesX) + nextX
                    );

                    if (closed[next] || !allowed(cells[next], from, to)) {
                        continue;
                    }

                    double penalty = (std::max)(
                        water_penalty(cells[current].water),
                        water_penalty(cells[next].water)
                    );

                    if (direction[0] != 0 && direction[1] != 0) {
                        const auto sideX = static_cast<std::uint32_t>(
                            z * static_cast<int>(terrain.samplesX) + nextX
                        );
                        const auto sideZ = static_cast<std::uint32_t>(
                            nextZ * static_cast<int>(terrain.samplesX) + x
                        );

                        if (!allowed(cells[sideX], from, to) ||
                            !allowed(cells[sideZ], from, to)) {
                            continue;
                        }

                        penalty = (std::max)(
                            penalty,
                            (std::max)(
                                water_penalty(cells[sideX].water),
                                water_penalty(cells[sideZ].water)
                            )
                        );
                    }

                    const double distance = terrain.spacingMeters *
                        std::hypot(
                            static_cast<double>(direction[0]),
                            static_cast<double>(direction[1])
                        );

                    const double grade = std::abs(
                        static_cast<double>(cells[next].height) -
                        cells[current].height
                    ) / distance;

                    const double terrainGrade = (std::max)(
                        static_cast<double>(cells[current].terrainGrade),
                        static_cast<double>(cells[next].terrainGrade)
                    );

                    const double candidate = costs[current] + distance *
                        (1.0 + 4.0 * grade + 24.0 * grade * grade +
                            8.0 * terrainGrade * terrainGrade + penalty);

                    if (candidate < costs[next]) {
                        costs[next] = candidate;
                        parents[next] = current;
                        pending.push({
                            candidate + heuristic(next, finish, terrain),
                            next
                        });
                    }
                }
            }

            if (!std::isfinite(costs[finish])) {
                throw std::runtime_error(
                    "Cannot route road around POI footprints"
                );
            }

            std::vector<std::uint32_t> chain;

            for (std::uint32_t cell = finish;
                 cell != noCell;
                 cell = parents[cell]) {
                chain.push_back(cell);
            }

            std::reverse(chain.begin(), chain.end());

            std::vector<RoadRoutePoint> route;
            route.reserve(chain.size() + 2);

            append_point(
                route,
                from.x,
                terrain.sample_height(from.x, from.z),
                from.z,
                water ? water->at_world(from.x, from.z) :
                    biome::WaterKind::Dry
            );

            for (const std::uint32_t cell : chain) {
                const float x = terrain.originX +
                    static_cast<float>(cell % terrain.samplesX) *
                    terrain.spacingMeters;
                const float z = terrain.originZ +
                    static_cast<float>(cell / terrain.samplesX) *
                    terrain.spacingMeters;

                append_point(route, x, cells[cell].height, z, cells[cell].water);
            }

            append_point(
                route,
                to.x,
                terrain.sample_height(to.x, to.z),
                to.z,
                water ? water->at_world(to.x, to.z) :
                    biome::WaterKind::Dry
            );

            if (route.size() < 2) {
                throw std::runtime_error("Road route has no length");
            }

            return route;
        }

    }

    void route_graph(
        RoadGraph& graph,
        const terrain::Heightfield& terrain,
        const biome::HydrologyLayer* water,
        const std::vector<settlement::POIPlacement>& pois
    ) {
        if (!terrain.valid() ||
            (water && (!water->valid() ||
                water->samplesX != terrain.samplesX ||
                water->samplesZ != terrain.samplesZ ||
                water->originX != terrain.originX ||
                water->originZ != terrain.originZ ||
                water->spacingMeters != terrain.spacingMeters))) {
            throw std::invalid_argument(
                "Road routing Terrain and Hydrology do not match"
            );
        }

        const auto cells = make_cells(terrain, water, pois);

        for (auto& edge : graph.edges) {
            if (edge.from >= graph.nodes.size() ||
                edge.to >= graph.nodes.size()) {
                throw std::invalid_argument("Invalid road edge");
            }

            edge.route = find_route(
                graph.nodes[edge.from],
                graph.nodes[edge.to],
                terrain,
                water,
                cells
            );
        }
    }

}
