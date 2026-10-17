#include "WorldEditor/Biome/Hydrology.h"

#include "WorldEditor/WorldGenerator/DeterministicRandom.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace world_editor::biome {

    namespace {

        constexpr float riverStepMeters = 16.0f;

        [[nodiscard]] std::size_t index_of(
            const HydrologyLayer& layer,
            const std::uint32_t x,
            const std::uint32_t z
        ) noexcept {
            return static_cast<std::size_t>(z) *
                layer.samplesX + x;
        }

        void paint_circle(
            HydrologyLayer& layer,
            const float worldX,
            const float worldZ,
            const float radius,
            const WaterKind kind
        ) {
            const float gx =
                (worldX - layer.originX) / layer.spacingMeters;

            const float gz =
                (worldZ - layer.originZ) / layer.spacingMeters;

            const float gridRadius =
                radius / layer.spacingMeters;

            const int minX = (std::max)(
                0,
                static_cast<int>(std::floor(gx - gridRadius))
            );

            const int minZ = (std::max)(
                0,
                static_cast<int>(std::floor(gz - gridRadius))
            );

            const int maxX = (std::min)(
                static_cast<int>(layer.samplesX) - 1,
                static_cast<int>(std::ceil(gx + gridRadius))
            );

            const int maxZ = (std::min)(
                static_cast<int>(layer.samplesZ) - 1,
                static_cast<int>(std::ceil(gz + gridRadius))
            );

            for (int z = minZ; z <= maxZ; ++z) {
                for (int x = minX; x <= maxX; ++x) {
                    const float dx =
                        (static_cast<float>(x) - gx) *
                        layer.spacingMeters;

                    const float dz =
                        (static_cast<float>(z) - gz) *
                        layer.spacingMeters;

                    if (dx * dx + dz * dz > radius * radius) {
                        continue;
                    }

                    WaterKind& cell = layer.cells[index_of(
                        layer,
                        static_cast<std::uint32_t>(x),
                        static_cast<std::uint32_t>(z)
                    )];

                    if (kind == WaterKind::Lake ||
                        cell == WaterKind::Dry) {
                        cell = kind;
                    }
                }
            }
        }

        [[nodiscard]] float coordinate(
            world_generator::DeterministicRandom& random,
            const float limit,
            const float margin
        ) {
            const float range = limit - 2.0f * margin;

            return margin + range *
                static_cast<float>(random.next()) /
                4294967296.0f;
        }

    }

    bool HydrologyLayer::valid() const noexcept {
        return samplesX >= 2 &&
            samplesZ >= 2 &&
            std::isfinite(originX) &&
            std::isfinite(originZ) &&
            std::isfinite(spacingMeters) &&
            spacingMeters > 0.0f &&
            cells.size() ==
                static_cast<std::size_t>(samplesX) *
                samplesZ;
    }

    WaterKind HydrologyLayer::at_world(
        const float x,
        const float z
    ) const noexcept {
        if (!valid() ||
            !std::isfinite(x) ||
            !std::isfinite(z)) {
            return WaterKind::Dry;
        }

        const float gx =
            (x - originX) / spacingMeters;

        const float gz =
            (z - originZ) / spacingMeters;

        if (gx < 0.0f || gz < 0.0f ||
            gx > static_cast<float>(samplesX - 1) ||
            gz > static_cast<float>(samplesZ - 1)) {
            return WaterKind::Dry;
        }

        const auto ix = static_cast<std::uint32_t>(
            std::round(gx)
        );

        const auto iz = static_cast<std::uint32_t>(
            std::round(gz)
        );

        return cells[index_of(*this, ix, iz)];
    }

    bool HydrologyLayer::blocks_poi(
        const float x,
        const float z
    ) const noexcept {
        if (!valid() ||
            !std::isfinite(x) ||
            !std::isfinite(z)) {
            return false;
        }

        // Conservative one-cell margin around wet areas.
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (at_world(
                    x + static_cast<float>(dx) *
                        spacingMeters,
                    z + static_cast<float>(dz) *
                        spacingMeters
                ) != WaterKind::Dry) {
                    return true;
                }
            }
        }

        return false;
    }

    HydrologyLayer generate_hydrology(
        const terrain::Heightfield& heightfield,
        const HydrologySettings& settings
    ) {
        if (!heightfield.valid() ||
            settings.lakeCount > 12 ||
            !std::isfinite(settings.riverWidthMeters) ||
            !std::isfinite(settings.marshWidthMeters) ||
            settings.riverWidthMeters < 16.0f ||
            settings.riverWidthMeters > 128.0f ||
            settings.marshWidthMeters < 0.0f ||
            settings.marshWidthMeters > 128.0f) {
            throw std::invalid_argument(
                "Invalid hydrology settings or terrain"
            );
        }

        HydrologyLayer layer;

        layer.samplesX = heightfield.samplesX;
        layer.samplesZ = heightfield.samplesZ;
        layer.originX = heightfield.originX;
        layer.originZ = heightfield.originZ;
        layer.spacingMeters = heightfield.spacingMeters;
        layer.settings = settings;

        layer.cells.resize(
            heightfield.heights.size(),
            WaterKind::Dry
        );

        world_generator::DeterministicRandom random(
            settings.seed
        );

        const float width =
            static_cast<float>(layer.samplesX - 1) *
            layer.spacingMeters;

        const float depth =
            static_cast<float>(layer.samplesZ - 1) *
            layer.spacingMeters;

        const bool alongZ = random.bounded(2) == 0;

        const float length = alongZ ? depth : width;
        const float crossLength = alongZ ? width : depth;

        // Generate a continuous river corridor.
        if (settings.riverEnabled) {
            const float middle = coordinate(
                random,
                crossLength,
                crossLength * 0.3f
            );

            const auto anchorCount =
                static_cast<std::uint32_t>(
                    std::ceil(length / 160.0f)
                ) + 1;

            std::vector<float> anchors(anchorCount);

            for (auto& anchor : anchors) {
                const float variation = (
                    static_cast<float>(random.next()) /
                    4294967296.0f - 0.5f
                ) * (std::min)(
                    crossLength * 0.24f,
                    160.0f
                );

                anchor = std::clamp(
                    middle + variation,
                    48.0f,
                    crossLength - 48.0f
                );
            }

            const auto pointCount =
                static_cast<std::uint32_t>(
                    std::ceil(length / riverStepMeters)
                );

            layer.river.reserve(pointCount + 1);

            for (std::uint32_t i = 0;
                 i <= pointCount;
                 ++i) {
                const float along =
                    length * static_cast<float>(i) /
                    static_cast<float>(pointCount);

                const float t =
                    along / length *
                    static_cast<float>(anchorCount - 1);

                const auto a = (std::min)(
                    static_cast<std::uint32_t>(t),
                    anchorCount - 2
                );

                const float f = t - static_cast<float>(a);
                const float smooth =
                    f * f * (3.0f - 2.0f * f);

                const float desired = std::lerp(
                    anchors[a],
                    anchors[a + 1],
                    smooth
                );

                float bestCross = desired;
                float bestScore =
                    std::numeric_limits<float>::infinity();

                // Prefer locally lower terrain.
                for (int shift = -3; shift <= 3; ++shift) {
                    const float cross = std::clamp(
                        desired +
                            static_cast<float>(shift) * 16.0f,
                        24.0f,
                        crossLength - 24.0f
                    );

                    const float x = layer.originX +
                        (alongZ ? cross : along);

                    const float z = layer.originZ +
                        (alongZ ? along : cross);

                    const float score =
                        heightfield.sample_height(x, z) +
                        std::abs(cross - desired) * 0.10f;

                    if (score < bestScore) {
                        bestScore = score;
                        bestCross = cross;
                    }
                }

                const float worldX = layer.originX +
                    (alongZ ? bestCross : along);

                const float worldZ = layer.originZ +
                    (alongZ ? along : bestCross);

                layer.river.push_back({
                    worldX,
                    heightfield.sample_height(
                        worldX,
                        worldZ
                    ),
                    worldZ
                });

                // Sweep disks along each segment. This avoids
                // gaps when successive points are far apart.
                if (layer.river.size() > 1) {
                    const WaterPoint& previous =
                        layer.river[layer.river.size() - 2];

                    const float dx = worldX - previous.x;
                    const float dz = worldZ - previous.z;

                    const int steps = (std::max)(
                        1,
                        static_cast<int>(std::ceil(
                            std::hypot(dx, dz) /
                            (layer.spacingMeters * 0.5f)
                        ))
                    );

                    for (int step = 0;
                         step <= steps;
                         ++step) {
                        const float fraction =
                            static_cast<float>(step) /
                            static_cast<float>(steps);

                        paint_circle(
                            layer,
                            std::lerp(
                                previous.x,
                                worldX,
                                fraction
                            ),
                            std::lerp(
                                previous.z,
                                worldZ,
                                fraction
                            ),
                            settings.riverWidthMeters * 0.5f,
                            WaterKind::River
                        );
                    }
                } else {
                    paint_circle(
                        layer,
                        worldX,
                        worldZ,
                        settings.riverWidthMeters * 0.5f,
                        WaterKind::River
                    );
                }
            }
        }

        // Place lakes using low-terrain candidates.
        for (std::uint32_t i = 0;
             i < settings.lakeCount;
             ++i) {
            const float margin =
                (std::min)(width, depth) * 0.12f;

            float bestX =
                layer.originX + width * 0.5f;

            float bestZ =
                layer.originZ + depth * 0.5f;

            float bestHeight =
                std::numeric_limits<float>::infinity();

            for (int trial = 0; trial < 24; ++trial) {
                const float x = layer.originX +
                    coordinate(random, width, margin);

                const float z = layer.originZ +
                    coordinate(random, depth, margin);

                const float height =
                    heightfield.sample_height(x, z);

                if (height < bestHeight) {
                    bestHeight = height;
                    bestX = x;
                    bestZ = z;
                }
            }

            const float radius = (std::min)(
                36.0f +
                    static_cast<float>(random.bounded(5)) *
                    12.0f,
                (std::min)(width, depth) * 0.11f
            );

            layer.lakes.push_back({
                {bestX, bestHeight, bestZ},
                radius
            });

            paint_circle(
                layer,
                bestX,
                bestZ,
                radius,
                WaterKind::Lake
            );
        }

        // Generate marshes around existing water.
        if (settings.marshWidthMeters > 0.0f) {
            const std::vector<WaterKind> core = layer.cells;

            const int radius = static_cast<int>(
                std::ceil(
                    settings.marshWidthMeters /
                    layer.spacingMeters
                )
            );

            const float squaredRange =
                settings.marshWidthMeters *
                settings.marshWidthMeters;

            for (std::uint32_t z = 0;
                 z < layer.samplesZ;
                 ++z) {
                for (std::uint32_t x = 0;
                     x < layer.samplesX;
                     ++x) {
                    if (core[index_of(layer, x, z)] ==
                        WaterKind::Dry) {
                        continue;
                    }

                    const int x0 = (std::max)(
                        0, static_cast<int>(x) - radius
                    );

                    const int x1 = (std::min)(
                        static_cast<int>(layer.samplesX) - 1,
                        static_cast<int>(x) + radius
                    );

                    const int z0 = (std::max)(
                        0, static_cast<int>(z) - radius
                    );

                    const int z1 = (std::min)(
                        static_cast<int>(layer.samplesZ) - 1,
                        static_cast<int>(z) + radius
                    );

                    for (int iz = z0; iz <= z1; ++iz) {
                        for (int ix = x0; ix <= x1; ++ix) {
                            const float dx =
                                static_cast<float>(
                                    ix - static_cast<int>(x)
                                ) * layer.spacingMeters;

                            const float dz =
                                static_cast<float>(
                                    iz - static_cast<int>(z)
                                ) * layer.spacingMeters;

                            if (dx * dx + dz * dz >
                                squaredRange) {
                                continue;
                            }

                            const auto targetIndex = index_of(
                                layer,
                                static_cast<std::uint32_t>(ix),
                                static_cast<std::uint32_t>(iz)
                            );

                            if (layer.cells[targetIndex] !=
                                WaterKind::Dry) {
                                continue;
                            }

                            const float wx = layer.originX +
                                static_cast<float>(ix) *
                                layer.spacingMeters;

                            const float wz = layer.originZ +
                                static_cast<float>(iz) *
                                layer.spacingMeters;

                            if (heightfield.slope_degrees(
                                wx, wz
                            ) <= 7.0f) {
                                layer.cells[targetIndex] =
                                    WaterKind::Marsh;
                            }
                        }
                    }
                }
            }

            // Sparse markers for inexpensive editor preview.
            for (std::uint32_t z = 0;
                 z < layer.samplesZ;
                 z += 4) {
                for (std::uint32_t x = 0;
                     x < layer.samplesX;
                     x += 4) {
                    if (layer.cells[index_of(layer, x, z)] !=
                        WaterKind::Marsh) {
                        continue;
                    }

                    layer.marshMarkers.push_back({
                        layer.originX +
                            static_cast<float>(x) *
                            layer.spacingMeters,
                        heightfield.height_at(x, z),
                        layer.originZ +
                            static_cast<float>(z) *
                            layer.spacingMeters
                    });
                }
            }
        }

        return layer;
    }

}
