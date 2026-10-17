#include "WorldEditor/Biome/Hydrology.h"

#include "WorldEditor/WorldGenerator/DeterministicRandom.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace world_editor::biome {

    namespace {

        using Random = world_generator::DeterministicRandom;

        constexpr float riverStep = 12.0f;
        constexpr float pi = std::numbers::pi_v<float>;

        struct Point2 {
            float x = 0.0f;
            float z = 0.0f;
        };

        struct RiverPreset {
            std::vector<Point2> main;
            std::vector<Point2> branch;
        };

        [[nodiscard]] std::size_t cell_index(
            const HydrologyLayer& layer,
            const std::uint32_t x,
            const std::uint32_t z
        ) noexcept {
            return static_cast<std::size_t>(z) *
                layer.samplesX + x;
        }

        [[nodiscard]] float random01(Random& random) {
            return static_cast<float>(random.next()) /
                4294967296.0f;
        }

        [[nodiscard]] float random_range(
            Random& random,
            const float minimum,
            const float maximum
        ) {
            return std::lerp(
                minimum,
                maximum,
                random01(random)
            );
        }

        [[nodiscard]] Point2 transform_point(
            Point2 p,
            const std::uint32_t rotation,
            const bool mirror
        ) {
            if (mirror) {
                p.x = 1.0f - p.x;
            }

            for (std::uint32_t i = 0; i < rotation; ++i) {
                p = {1.0f - p.z, p.x};
            }

            return p;
        }

        [[nodiscard]] RiverPreset river_preset(
            const RiverShape shape
        ) {
            switch (shape) {
            case RiverShape::Straight:
                return {{
                    {0.0f, 0.44f},
                    {0.28f, 0.40f},
                    {0.65f, 0.54f},
                    {1.0f, 0.48f}
                }, {}};

            case RiverShape::LShape:
                return {{
                    {0.0f, 0.22f},
                    {0.38f, 0.23f},
                    {0.54f, 0.36f},
                    {0.57f, 0.70f},
                    {0.56f, 1.0f}
                }, {}};

            case RiverShape::UShape:
                return {{
                    {0.19f, 0.0f},
                    {0.20f, 0.43f},
                    {0.35f, 0.73f},
                    {0.63f, 0.74f},
                    {0.80f, 0.49f},
                    {0.79f, 0.0f}
                }, {}};

            case RiverShape::XShape:
                return {{
                    {0.0f, 0.44f},
                    {0.34f, 0.47f},
                    {0.50f, 0.50f},
                    {0.72f, 0.53f},
                    {1.0f, 0.55f}
                }, {
                    {0.45f, 0.0f},
                    {0.48f, 0.29f},
                    {0.50f, 0.50f},
                    {0.52f, 0.72f},
                    {0.55f, 1.0f}
                }};

            default:
                return river_preset(RiverShape::Straight);
            }
        }

        [[nodiscard]] Point2 catmull_rom(
            const Point2 p0,
            const Point2 p1,
            const Point2 p2,
            const Point2 p3,
            const float t
        ) {
            const float t2 = t * t;
            const float t3 = t2 * t;

            const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
            const float h10 = t3 - 2.0f * t2 + t;
            const float h01 = -2.0f * t3 + 3.0f * t2;
            const float h11 = t3 - t2;

            const Point2 m1{
                (p2.x - p0.x) * 0.5f,
                (p2.z - p0.z) * 0.5f
            };

            const Point2 m2{
                (p3.x - p1.x) * 0.5f,
                (p3.z - p1.z) * 0.5f
            };

            return {
                h00 * p1.x + h10 * m1.x +
                    h01 * p2.x + h11 * m2.x,

                h00 * p1.z + h10 * m1.z +
                    h01 * p2.z + h11 * m2.z
            };
        }

        void paint_circle(
            HydrologyLayer& layer,
            const float cx,
            const float cz,
            const float radius,
            const WaterKind kind
        ) {
            const float gx =
                (cx - layer.originX) / layer.spacingMeters;

            const float gz =
                (cz - layer.originZ) / layer.spacingMeters;

            const float r = radius / layer.spacingMeters;

            const int minX = (std::max)(
                0, static_cast<int>(std::floor(gx - r))
            );

            const int minZ = (std::max)(
                0, static_cast<int>(std::floor(gz - r))
            );

            const int maxX = (std::min)(
                static_cast<int>(layer.samplesX) - 1,
                static_cast<int>(std::ceil(gx + r))
            );

            const int maxZ = (std::min)(
                static_cast<int>(layer.samplesZ) - 1,
                static_cast<int>(std::ceil(gz + r))
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

                    WaterKind& cell = layer.cells[cell_index(
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

        [[nodiscard]] Point2 choose_low_terrain_point(
            const terrain::Heightfield& heightfield,
            const float minX,
            const float maxX,
            const float minZ,
            const float maxZ,
            const Point2& previous,
            const Point2& desired,
            const Point2& forward,
            const float searchRadius,
            const float step
        ) {
            Point2 best = desired;
            float bestScore =
                std::numeric_limits<float>::infinity();

            const float length = std::hypot(
                forward.x,
                forward.z
            );

            Point2 dir = forward;

            if (length > 0.001f) {
                dir.x /= length;
                dir.z /= length;
            } else {
                dir = {1.0f, 0.0f};
            }

            const Point2 normal{
                -dir.z,
                dir.x
            };

            for (int forwardOffset = -2;
                 forwardOffset <= 2;
                 ++forwardOffset) {
                for (int lateral = -8;
                     lateral <= 8;
                     ++lateral) {
                    Point2 candidate{
                        desired.x +
                            dir.x * (
                                static_cast<float>(
                                    forwardOffset
                                ) * step * 0.75f
                            ) +
                            normal.x * (
                                static_cast<float>(
                                    lateral
                                ) * step
                            ),

                        desired.z +
                            dir.z * (
                                static_cast<float>(
                                    forwardOffset
                                ) * step * 0.75f
                            ) +
                            normal.z * (
                                static_cast<float>(
                                    lateral
                                ) * step
                            )
                    };

                    candidate.x = std::clamp(
                        candidate.x,
                        minX,
                        maxX
                    );

                    candidate.z = std::clamp(
                        candidate.z,
                        minZ,
                        maxZ
                    );

                    const float dxDesired =
                        candidate.x - desired.x;

                    const float dzDesired =
                        candidate.z - desired.z;

                    if (dxDesired * dxDesired +
                        dzDesired * dzDesired >
                        searchRadius * searchRadius) {
                        continue;
                    }

                    const float dxPrev =
                        candidate.x - previous.x;

                    const float dzPrev =
                        candidate.z - previous.z;

                    const float progress =
                        dxPrev * dir.x + dzPrev * dir.z;

                    if (progress < step * 0.15f) {
                        continue;
                    }

                    const float height =
                        heightfield.sample_height(
                            candidate.x,
                            candidate.z
                        );

                    const float slope =
                        heightfield.slope_degrees(
                            candidate.x,
                            candidate.z
                        );

                    const float distanceToDesired =
                        std::hypot(dxDesired, dzDesired);

                    const float score =
                        height +
                        slope * 14.0f +
                        distanceToDesired * 0.22f;

                    if (score < bestScore) {
                        bestScore = score;
                        best = candidate;
                    }
                }
            }

            return best;
        }

        void smooth_channel(
            std::vector<WaterPoint>& channel
        ) {
            if (channel.size() < 3) {
                return;
            }

            for (int pass = 0; pass < 2; ++pass) {
                std::vector<WaterPoint> copy = channel;

                for (std::size_t i = 1;
                     i + 1 < channel.size();
                     ++i) {
                    channel[i].x = (
                        copy[i - 1].x +
                        copy[i].x * 2.0f +
                        copy[i + 1].x
                    ) * 0.25f;

                    channel[i].z = (
                        copy[i - 1].z +
                        copy[i].z * 2.0f +
                        copy[i + 1].z
                    ) * 0.25f;

                    channel[i].y = (
                        copy[i - 1].y +
                        copy[i].y * 2.0f +
                        copy[i + 1].y
                    ) * 0.25f;
                }
            }
        }

        [[nodiscard]] std::vector<WaterPoint> build_channel(
            HydrologyLayer& layer,
            const terrain::Heightfield& heightfield,
            const std::vector<Point2>& control,
            Random& random,
            const std::uint32_t rotation,
            const bool mirror
        ) {
            std::vector<WaterPoint> result;

            if (control.size() < 2) {
                return result;
            }

            const float width =
                static_cast<float>(layer.samplesX - 1) *
                layer.spacingMeters;

            const float depth =
                static_cast<float>(layer.samplesZ - 1) *
                layer.spacingMeters;

            std::vector<Point2> points;
            points.reserve(control.size());

            for (const Point2 p : control) {
                const Point2 transformed = transform_point(
                    p, rotation, mirror
                );

                points.push_back({
                    layer.originX + transformed.x * width,
                    layer.originZ + transformed.z * depth
                });
            }

            const float phase = random_range(
                random, 0.0f, 2.0f * pi
            );

            const float amplitude = (std::min)(
                10.0f,
                (std::min)(width, depth) * 0.010f
            );

            const float mapMinX = layer.originX + 6.0f;
            const float mapMaxX =
                layer.originX + width - 6.0f;

            const float mapMinZ = layer.originZ + 6.0f;
            const float mapMaxZ =
                layer.originZ + depth - 6.0f;

            const float searchRadius = std::clamp(
                (std::min)(width, depth) * 0.10f,
                72.0f,
                240.0f
            );

            const float step = std::clamp(
                layer.spacingMeters * 2.0f,
                12.0f,
                24.0f
            );

            const std::size_t segments = points.size() - 1;

            for (std::size_t i = 0; i < segments; ++i) {
                const Point2 p0 =
                    points[i == 0 ? i : i - 1];

                const Point2 p1 = points[i];
                const Point2 p2 = points[i + 1];

                const Point2 p3 =
                    points[(std::min)(i + 2, segments)];

                const float dx = p2.x - p1.x;
                const float dz = p2.z - p1.z;

                const float length = std::hypot(dx, dz);

                const auto steps = (std::max)(
                    1,
                    static_cast<int>(
                        std::ceil(length / riverStep)
                    )
                );

                for (int sample = 0;
                     sample <= steps;
                     ++sample) {
                    if (i != 0 && sample == 0) {
                        continue;
                    }

                    const float t =
                        static_cast<float>(sample) /
                        static_cast<float>(steps);

                    Point2 raw = catmull_rom(
                        p0, p1, p2, p3, t
                    );

                    const float progress = (
                        static_cast<float>(i) + t
                    ) / static_cast<float>(segments);

                    const float wave =
                        std::sin(t * pi) *
                        std::sin(
                            progress * 6.0f * pi + phase
                        ) * amplitude;

                    if (length > 0.001f) {
                        raw.x += (-dz / length) * wave;
                        raw.z += (dx / length) * wave;
                    }

                    raw.x = std::clamp(
                        raw.x,
                        mapMinX,
                        mapMaxX
                    );

                    raw.z = std::clamp(
                        raw.z,
                        mapMinZ,
                        mapMaxZ
                    );

                    Point2 finalPoint = raw;

                    if (!result.empty()) {
                        Point2 previous{
                            result.back().x,
                            result.back().z
                        };

                        Point2 forward{
                            raw.x - previous.x,
                            raw.z - previous.z
                        };

                        finalPoint = choose_low_terrain_point(
                            heightfield,
                            mapMinX,
                            mapMaxX,
                            mapMinZ,
                            mapMaxZ,
                            previous,
                            raw,
                            forward,
                            searchRadius,
                            step
                        );
                    }

                    result.push_back({
                        finalPoint.x,
                        heightfield.sample_height(
                            finalPoint.x,
                            finalPoint.z
                        ),
                        finalPoint.z
                    });
                }
            }

            smooth_channel(result);

            const float riverRadius =
                layer.settings.riverWidthMeters * 0.5f;

            for (std::size_t i = 0; i < result.size(); ++i) {
                if (i == 0) {
                    paint_circle(
                        layer,
                        result[i].x,
                        result[i].z,
                        riverRadius,
                        WaterKind::River
                    );
                    continue;
                }

                const WaterPoint& a = result[i - 1];
                const WaterPoint& b = result[i];

                const float distance =
                    std::hypot(b.x - a.x, b.z - a.z);

                const int subdivisions = (std::max)(
                    1,
                    static_cast<int>(std::ceil(
                        distance /
                        (layer.spacingMeters * 0.5f)
                    ))
                );

                for (int s = 0; s <= subdivisions; ++s) {
                    const float f =
                        static_cast<float>(s) /
                        static_cast<float>(subdivisions);

                    paint_circle(
                        layer,
                        std::lerp(a.x, b.x, f),
                        std::lerp(a.z, b.z, f),
                        riverRadius,
                        WaterKind::River
                    );
                }
            }

            return result;
        }

        [[nodiscard]] bool far_from_rivers(
            const HydrologyLayer& layer,
            const float x,
            const float z,
            const float radius
        ) {
            const float minimum =
                radius +
                layer.settings.riverWidthMeters * 0.5f +
                layer.settings.marshWidthMeters + 24.0f;

            const float distanceSquared = minimum * minimum;

            const auto clear_channel =
                [x, z, distanceSquared](
                    const std::vector<WaterPoint>& channel
                ) {
                    for (const auto& point : channel) {
                        const float dx = x - point.x;
                        const float dz = z - point.z;

                        if (dx * dx + dz * dz <
                            distanceSquared) {
                            return false;
                        }
                    }

                    return true;
                };

            if (!clear_channel(layer.river)) {
                return false;
            }

            for (const auto& branch : layer.tributaries) {
                if (!clear_channel(branch)) {
                    return false;
                }
            }

            return true;
        }

        void generate_lakes(
            HydrologyLayer& layer,
            const terrain::Heightfield& heightfield,
            Random& random
        ) {
            const float width =
                static_cast<float>(layer.samplesX - 1) *
                layer.spacingMeters;

            const float depth =
                static_cast<float>(layer.samplesZ - 1) *
                layer.spacingMeters;

            const float minimumSide = (std::min)(width, depth);

            const float separation = (std::max)(
                layer.settings.lakeSeparationMeters,
                60.0f
            );

            for (std::uint32_t i = 0;
                 i < layer.settings.lakeCount;
                 ++i) {
                bool found = false;

                LakeArea best{};
                float bestScore =
                    std::numeric_limits<float>::infinity();

                for (int attempt = 0; attempt < 256; ++attempt) {
                    const float radius = (std::min)(
                        random_range(random, 28.0f, 72.0f),
                        minimumSide * 0.09f
                    );

                    const float margin =
                        radius +
                        layer.settings.marshWidthMeters +
                        16.0f;

                    if (margin * 2.0f >= minimumSide) {
                        continue;
                    }

                    const float x = layer.originX +
                        random_range(
                            random, margin, width - margin
                        );

                    const float z = layer.originZ +
                        random_range(
                            random, margin, depth - margin
                        );

                    const float slope =
                        heightfield.slope_degrees(x, z);

                    if (slope > 8.0f) {
                        continue;
                    }

                    bool available = true;

                    for (const auto& other : layer.lakes) {
                        const float dx =
                            x - other.center.x;

                        const float dz =
                            z - other.center.z;

                        const float required = (std::max)(
                            separation,
                            radius +
                            other.radiusMeters +
                            layer.settings.marshWidthMeters +
                            20.0f
                        );

                        if (dx * dx + dz * dz <
                            required * required) {
                            available = false;
                            break;
                        }
                    }

                    if (!available ||
                        !far_from_rivers(
                            layer, x, z, radius
                        )) {
                        continue;
                    }

                    const float height =
                        heightfield.sample_height(x, z);

                    const float score =
                        height +
                        random_range(random, 0.0f, 18.0f);

                    if (score < bestScore) {
                        bestScore = score;
                        best.center = {x, height, z};
                        best.radiusMeters = radius;
                        found = true;
                    }
                }

                if (!found) {
                    throw std::runtime_error(
                        "Cannot place all lakes. "
                        "Reduce lake count, river width "
                        "or minimum lake separation."
                    );
                }

                layer.lakes.push_back(best);

                paint_circle(
                    layer,
                    best.center.x,
                    best.center.z,
                    best.radiusMeters,
                    WaterKind::Lake
                );
            }
        }

        void generate_marshes(
            HydrologyLayer& layer,
            const terrain::Heightfield& heightfield
        ) {
            if (layer.settings.marshWidthMeters <= 0.0f) {
                return;
            }

            const std::vector<WaterKind> core = layer.cells;

            const int radius = static_cast<int>(std::ceil(
                layer.settings.marshWidthMeters /
                layer.spacingMeters
            ));

            const float maxDistanceSquared =
                layer.settings.marshWidthMeters *
                layer.settings.marshWidthMeters;

            for (std::uint32_t z = 0;
                 z < layer.samplesZ;
                 ++z) {
                for (std::uint32_t x = 0;
                     x < layer.samplesX;
                     ++x) {
                    if (core[cell_index(layer, x, z)] ==
                        WaterKind::Dry) {
                        continue;
                    }

                    for (int dz = -radius; dz <= radius; ++dz) {
                        for (int dx = -radius; dx <= radius; ++dx) {
                            const int tx =
                                static_cast<int>(x) + dx;

                            const int tz =
                                static_cast<int>(z) + dz;

                            if (tx < 0 || tz < 0 ||
                                tx >= static_cast<int>(
                                    layer.samplesX
                                ) ||
                                tz >= static_cast<int>(
                                    layer.samplesZ
                                )) {
                                continue;
                            }

                            const float distanceSquared =
                                static_cast<float>(dx * dx + dz * dz) *
                                layer.spacingMeters *
                                layer.spacingMeters;

                            if (distanceSquared >
                                maxDistanceSquared) {
                                continue;
                            }

                            const auto index = cell_index(
                                layer,
                                static_cast<std::uint32_t>(tx),
                                static_cast<std::uint32_t>(tz)
                            );

                            if (layer.cells[index] !=
                                WaterKind::Dry) {
                                continue;
                            }

                            const float worldX =
                                layer.originX +
                                static_cast<float>(tx) *
                                layer.spacingMeters;

                            const float worldZ =
                                layer.originZ +
                                static_cast<float>(tz) *
                                layer.spacingMeters;

                            if (heightfield.slope_degrees(
                                worldX, worldZ
                            ) <= 7.0f) {
                                layer.cells[index] =
                                    WaterKind::Marsh;
                            }
                        }
                    }
                }
            }

            for (std::uint32_t z = 0;
                 z < layer.samplesZ;
                 z += 4) {
                for (std::uint32_t x = 0;
                     x < layer.samplesX;
                     x += 4) {
                    if (layer.cells[cell_index(layer, x, z)] !=
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

        return cells[cell_index(*this, ix, iz)];
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
            static_cast<std::uint32_t>(
                settings.riverShape
            ) > 4 ||
            !std::isfinite(settings.riverWidthMeters) ||
            !std::isfinite(settings.marshWidthMeters) ||
            !std::isfinite(settings.lakeSeparationMeters) ||
            settings.riverWidthMeters < 16.0f ||
            settings.riverWidthMeters > 128.0f ||
            settings.marshWidthMeters < 0.0f ||
            settings.marshWidthMeters > 128.0f ||
            settings.lakeSeparationMeters < 0.0f ||
            settings.lakeSeparationMeters > 2048.0f) {
            throw std::invalid_argument(
                "Invalid Hydrology configuration"
            );
        }

        HydrologyLayer layer;

        layer.samplesX = heightfield.samplesX;
        layer.samplesZ = heightfield.samplesZ;
        layer.originX = heightfield.originX;
        layer.originZ = heightfield.originZ;
        layer.spacingMeters = heightfield.spacingMeters;
        layer.settings = settings;

        layer.cells.assign(
            heightfield.heights.size(),
            WaterKind::Dry
        );

        Random random(settings.seed);

        if (settings.riverEnabled) {
            RiverShape shape = settings.riverShape;

            if (shape == RiverShape::Auto) {
                shape = static_cast<RiverShape>(
                    1u + random.bounded(4)
                );
            }

            RiverPreset preset = river_preset(shape);

            const auto rotation = random.bounded(4);
            const bool mirror = random.bounded(2) == 0;

            layer.river = build_channel(
                layer,
                heightfield,
                preset.main,
                random,
                rotation,
                mirror
            );

            if (!preset.branch.empty()) {
                layer.tributaries.push_back(build_channel(
                    layer,
                    heightfield,
                    preset.branch,
                    random,
                    rotation,
                    mirror
                ));
            }
        }

        generate_lakes(layer, heightfield, random);
        generate_marshes(layer, heightfield);

        return layer;
    }

}
