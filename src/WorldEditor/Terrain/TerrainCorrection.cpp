#include "WorldEditor/Terrain/TerrainCorrection.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace world_editor::terrain {

    namespace {

        constexpr float roadHalfWidth = 7.0f;
        constexpr float roadShoulder = 24.0f;
        constexpr float bridgeClearance = 2.5f;
        constexpr float bridgeApproach = 64.0f;

        [[nodiscard]] float smooth(float value) noexcept {
            const float t = std::clamp(value, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] bool wet(
            const biome::HydrologyLayer* water,
            const float x,
            const float z
        ) noexcept {
            if (!water) {
                return false;
            }

            const auto kind = water->at_world(x, z);
            return kind == biome::WaterKind::River ||
                kind == biome::WaterKind::Lake;
        }

        [[nodiscard]] std::size_t index(
            const Heightfield& terrain,
            const std::uint32_t x,
            const std::uint32_t z
        ) noexcept {
            return static_cast<std::size_t>(z) * terrain.samplesX + x;
        }

        [[nodiscard]] float world_x(
            const Heightfield& terrain,
            const std::uint32_t x
        ) noexcept {
            return terrain.originX + static_cast<float>(x) * terrain.spacingMeters;
        }

        [[nodiscard]] float world_z(
            const Heightfield& terrain,
            const std::uint32_t z
        ) noexcept {
            return terrain.originZ + static_cast<float>(z) * terrain.spacingMeters;
        }

        [[nodiscard]] float platform_height(
            const Heightfield& terrain,
            const settlement::POIPlacement& poi
        ) {
            std::vector<float> heights;

            for (std::uint32_t z = 0; z < terrain.samplesZ; ++z) {
                const float wz = world_z(terrain, z);

                if (std::abs(wz - poi.centerZ) > poi.depthMeters * 0.5f) {
                    continue;
                }

                for (std::uint32_t x = 0; x < terrain.samplesX; ++x) {
                    if (std::abs(world_x(terrain, x) - poi.centerX) <=
                        poi.widthMeters * 0.5f) {
                        heights.push_back(terrain.heights[index(terrain, x, z)]);
                    }
                }
            }

            if (heights.empty()) {
                return terrain.sample_height(poi.centerX, poi.centerZ);
            }

            const auto middle = heights.begin() + heights.size() / 2;
            std::nth_element(heights.begin(), middle, heights.end());
            return *middle;
        }

        void shape_roads(
            Heightfield& result,
            const Heightfield& source,
            world_generator::MapLayout& layout,
            const biome::HydrologyLayer* water
        ) {
            std::vector<float> weighted(result.heights.size(), 0.0f);
            std::vector<float> weights(result.heights.size(), 0.0f);

            for (auto& edge : layout.roads.edges) {
                auto& route = edge.route;

                if (route.size() < 2) {
                    continue;
                }

                std::vector<float> profile(route.size());

                for (std::size_t i = 0; i < route.size(); ++i) {
                    float sum = 0.0f;
                    float count = 0.0f;

                    for (std::size_t j = 0; j < route.size(); ++j) {
                        if (std::abs(route[j].distanceMeters -
                            route[i].distanceMeters) <= 24.0f &&
                            route[j].water != biome::WaterKind::River &&
                            route[j].water != biome::WaterKind::Lake) {
                            sum += route[j].y;
                            count += 1.0f;
                        }
                    }

                    profile[i] = count > 0.0f ? sum / count : route[i].y;
                }

                for (std::size_t i = 0; i < route.size();) {
                    if (route[i].water != biome::WaterKind::River &&
                        route[i].water != biome::WaterKind::Lake) {
                        ++i;
                        continue;
                    }

                    const std::size_t first = i;

                    while (i < route.size() &&
                        (route[i].water == biome::WaterKind::River ||
                            route[i].water == biome::WaterKind::Lake)) {
                        ++i;
                    }

                    if (first == 0 || i == route.size()) {
                        continue;
                    }

                    const float deck = std::max(
                        profile[first - 1],
                        profile[i]
                    ) + bridgeClearance;

                    for (std::size_t j = first; j < i; ++j) {
                        profile[j] = deck;
                    }

                    for (std::size_t j = first; j-- > 0;) {
                        const float distance = route[first].distanceMeters -
                            route[j].distanceMeters;

                        if (distance > bridgeApproach) {
                            break;
                        }

                        profile[j] = std::lerp(
                            profile[j],
                            deck,
                            smooth(1.0f - distance / bridgeApproach)
                        );
                    }

                    for (std::size_t j = i; j < route.size(); ++j) {
                        const float distance = route[j].distanceMeters -
                            route[i - 1].distanceMeters;

                        if (distance > bridgeApproach) {
                            break;
                        }

                        profile[j] = std::lerp(
                            profile[j],
                            deck,
                            smooth(1.0f - distance / bridgeApproach)
                        );
                    }
                }

                for (std::size_t segment = 1; segment < route.size(); ++segment) {
                    const auto& a = route[segment - 1];
                    const auto& b = route[segment];
                    const float dx = b.x - a.x;
                    const float dz = b.z - a.z;
                    const float lengthSquared = dx * dx + dz * dz;

                    if (lengthSquared < 0.01f) {
                        continue;
                    }

                    const float radius = roadHalfWidth + roadShoulder;
                    const auto minX = static_cast<int>(std::floor(
                        (std::min(a.x, b.x) - radius - source.originX) /
                        source.spacingMeters
                    ));
                    const auto maxX = static_cast<int>(std::ceil(
                        (std::max(a.x, b.x) + radius - source.originX) /
                        source.spacingMeters
                    ));
                    const auto minZ = static_cast<int>(std::floor(
                        (std::min(a.z, b.z) - radius - source.originZ) /
                        source.spacingMeters
                    ));
                    const auto maxZ = static_cast<int>(std::ceil(
                        (std::max(a.z, b.z) + radius - source.originZ) /
                        source.spacingMeters
                    ));

                    for (int z = std::max(0, minZ);
                         z <= std::min(static_cast<int>(source.samplesZ) - 1, maxZ);
                         ++z) {
                        for (int x = std::max(0, minX);
                             x <= std::min(static_cast<int>(source.samplesX) - 1, maxX);
                             ++x) {
                            const float wx = world_x(source, static_cast<std::uint32_t>(x));
                            const float wz = world_z(source, static_cast<std::uint32_t>(z));

                            if (wet(water, wx, wz)) {
                                continue;
                            }

                            const float t = std::clamp(
                                ((wx - a.x) * dx + (wz - a.z) * dz) /
                                    lengthSquared,
                                0.0f,
                                1.0f
                            );
                            const float distance = std::hypot(
                                wx - std::lerp(a.x, b.x, t),
                                wz - std::lerp(a.z, b.z, t)
                            );

                            if (distance >= radius) {
                                continue;
                            }

                            const float influence = distance <= roadHalfWidth
                                ? 1.0f
                                : 1.0f - smooth(
                                    (distance - roadHalfWidth) / roadShoulder
                                );
                            const auto cell = index(
                                source,
                                static_cast<std::uint32_t>(x),
                                static_cast<std::uint32_t>(z)
                            );

                            weighted[cell] += std::lerp(
                                profile[segment - 1],
                                profile[segment],
                                t
                            ) * influence;
                            weights[cell] += influence;
                        }
                    }
                }

                for (std::size_t i = 0; i < route.size(); ++i) {
                    route[i].y = profile[i];
                }
            }

            for (std::size_t cell = 0; cell < result.heights.size(); ++cell) {
                if (weights[cell] > 0.0f) {
                    result.heights[cell] = std::lerp(
                        source.heights[cell],
                        weighted[cell] / weights[cell],
                        std::clamp(weights[cell], 0.0f, 1.0f)
                    );
                }
            }
        }

        void shape_pois(
            Heightfield& result,
            const Heightfield& source,
            world_generator::MapLayout& layout,
            const biome::HydrologyLayer* water
        ) {
            std::vector<float> weights(result.heights.size(), 0.0f);
            std::vector<float> targets(result.heights.size(), 0.0f);
            std::vector<float> platforms(
                result.heights.size(),
                std::numeric_limits<float>::quiet_NaN()
            );

            for (auto& poi : layout.pois) {
                const float height = platform_height(source, poi);
                poi.centerY = height;

                float greatestDifference = 0.0f;

                for (std::uint32_t z = 0; z < source.samplesZ; ++z) {
                    const float dz = std::max(
                        0.0f,
                        std::abs(world_z(source, z) - poi.centerZ) -
                            poi.depthMeters * 0.5f
                    );

                    if (dz > 128.0f) {
                        continue;
                    }

                    for (std::uint32_t x = 0; x < source.samplesX; ++x) {
                        const float dx = std::max(
                            0.0f,
                            std::abs(world_x(source, x) - poi.centerX) -
                                poi.widthMeters * 0.5f
                        );

                        if (std::hypot(dx, dz) > 128.0f ||
                            wet(water, world_x(source, x), world_z(source, z))) {
                            continue;
                        }

                        greatestDifference = std::max(
                            greatestDifference,
                            std::abs(source.heights[index(source, x, z)] - height)
                        );
                    }
                }

                const float transition = std::clamp(
                    48.0f + greatestDifference * 5.0f,
                    64.0f,
                    512.0f
                );

                for (std::uint32_t z = 0; z < source.samplesZ; ++z) {
                    const float dz = std::max(
                        0.0f,
                        std::abs(world_z(source, z) - poi.centerZ) -
                            poi.depthMeters * 0.5f
                    );

                    if (dz >= transition) {
                        continue;
                    }

                    for (std::uint32_t x = 0; x < source.samplesX; ++x) {
                        const float wx = world_x(source, x);
                        const float wz = world_z(source, z);
                        const float dx = std::max(
                            0.0f,
                            std::abs(wx - poi.centerX) -
                                poi.widthMeters * 0.5f
                        );
                        const float distance = std::hypot(dx, dz);

                        if (distance >= transition || wet(water, wx, wz)) {
                            continue;
                        }

                        const auto cell = index(source, x, z);
                        const float influence = 1.0f - smooth(
                            distance / transition
                        );

                        weights[cell] += influence;
                        targets[cell] += height * influence;

                        if (distance == 0.0f) {
                            platforms[cell] = height;
                        }
                    }
                }
            }

            for (std::size_t cell = 0; cell < result.heights.size(); ++cell) {
                if (std::isfinite(platforms[cell])) {
                    result.heights[cell] = platforms[cell];
                } else if (weights[cell] > 0.0f) {
                    result.heights[cell] = std::lerp(
                        result.heights[cell],
                        targets[cell] / weights[cell],
                        std::clamp(weights[cell], 0.0f, 1.0f)
                    );
                }
            }
        }

        void refresh_layout(
            const Heightfield& terrain,
            world_generator::MapLayout& layout
        ) {
            for (auto& exit : layout.exits) {
                exit.y = terrain.sample_height(exit.x, exit.z);
            }

            for (auto& edge : layout.roads.edges) {
                auto& route = edge.route;

                for (std::size_t i = 0; i < route.size(); ++i) {
                    if (route[i].water != biome::WaterKind::River &&
                        route[i].water != biome::WaterKind::Lake) {
                        route[i].y = terrain.sample_height(
                            route[i].x,
                            route[i].z
                        );
                    }

                    if (i == 0) {
                        route[i].slopeDegrees = 0.0f;
                        continue;
                    }

                    const float distance = std::hypot(
                        route[i].x - route[i - 1].x,
                        route[i].z - route[i - 1].z
                    );
                    route[i].slopeDegrees = std::atan2(
                        std::abs(route[i].y - route[i - 1].y),
                        std::max(distance, 0.001f)
                    ) * (180.0f / std::numbers::pi_v<float>);
                }
            }
        }

    }

    Heightfield correct_for_layout(
        const Heightfield& source,
        world_generator::MapLayout& layout,
        const biome::HydrologyLayer* water
    ) {
        if (!source.valid() ||
            (water && (!water->valid() ||
                water->samplesX != source.samplesX ||
                water->samplesZ != source.samplesZ ||
                water->originX != source.originX ||
                water->originZ != source.originZ ||
                water->spacingMeters != source.spacingMeters))) {
            throw std::invalid_argument(
                "Terrain correction requires matching Terrain and Hydrology"
            );
        }

        Heightfield result = source;
        shape_roads(result, source, layout, water);
        shape_pois(result, source, layout, water);
        refresh_layout(result, layout);
        return result;
    }

}
