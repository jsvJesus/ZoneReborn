#include "WorldEditor/Biome/WaterSurface.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace world_editor::biome {

    namespace {

        constexpr float textureScale = 16.0f;

        [[nodiscard]] WaterPoint blend(
            const WaterPoint& a,
            const WaterPoint& b,
            float t
        ) noexcept {
            return {
                std::lerp(a.x, b.x, t),
                std::lerp(a.y, b.y, t),
                std::lerp(a.z, b.z, t)
            };
        }

        [[nodiscard]] std::vector<WaterPoint> smooth_channel(
            const std::vector<WaterPoint>& channel
        ) {
            std::vector<WaterPoint> points = channel;

            for (int pass = 0; pass < 2; ++pass) {
                std::vector<WaterPoint> refined;
                refined.reserve(points.size() * 2);
                refined.push_back(points.front());

                for (std::size_t i = 1; i < points.size(); ++i) {
                    refined.push_back(blend(points[i - 1], points[i], 0.25f));
                    refined.push_back(blend(points[i - 1], points[i], 0.75f));
                }

                refined.push_back(points.back());
                points = std::move(refined);
            }

            return points;
        }

        void append_channel(
            engine::renderer::SurfaceMesh& mesh,
            const std::vector<WaterPoint>& channel,
            float width
        ) {
            if (channel.size() < 2) {
                return;
            }

            const auto points = smooth_channel(channel);
            const auto start = static_cast<std::uint32_t>(mesh.vertices.size());

            for (std::size_t i = 0; i < points.size(); ++i) {
                const WaterPoint& point = points[i];
                const WaterPoint& previous = points[i > 0 ? i - 1 : i];
                const WaterPoint& next = points[
                    i + 1 < points.size() ? i + 1 : i
                ];

                const float inX = point.x - previous.x;
                const float inZ = point.z - previous.z;
                const float outX = next.x - point.x;
                const float outZ = next.z - point.z;
                const float inLength = std::hypot(inX, inZ);
                const float outLength = std::hypot(outX, outZ);
                float sideX = 0.0f;
                float sideZ = 1.0f;

                if (inLength > 0.001f && outLength > 0.001f) {
                    const float normalInX = -inZ / inLength;
                    const float normalInZ = inX / inLength;
                    const float normalOutX = -outZ / outLength;
                    const float normalOutZ = outX / outLength;
                    const float joinX = normalInX + normalOutX;
                    const float joinZ = normalInZ + normalOutZ;
                    const float joinLength = std::hypot(joinX, joinZ);

                    if (joinLength > 0.001f) {
                        const float projection = (
                            joinX * normalOutX + joinZ * normalOutZ
                        ) / joinLength;
                        const float scale = std::min(
                            1.25f,
                            1.0f / std::max(0.7f, projection)
                        );
                        sideX = joinX / joinLength * scale;
                        sideZ = joinZ / joinLength * scale;
                    }
                } else if (outLength > 0.001f) {
                    sideX = -outZ / outLength;
                    sideZ = outX / outLength;
                } else if (inLength > 0.001f) {
                    sideX = -inZ / inLength;
                    sideZ = inX / inLength;
                }

                const float offsetX = sideX * width * 0.5f;
                const float offsetZ = sideZ * width * 0.5f;
                const float y = point.y + 0.45f;

                mesh.vertices.push_back({
                    point.x - offsetX,
                    y,
                    point.z - offsetZ,
                    (point.x - offsetX) / textureScale,
                    (point.z - offsetZ) / textureScale
                });
                mesh.vertices.push_back({
                    point.x + offsetX,
                    y,
                    point.z + offsetZ,
                    (point.x + offsetX) / textureScale,
                    (point.z + offsetZ) / textureScale
                });
            }

            for (std::size_t i = 1; i < points.size(); ++i) {
                const auto a = start + static_cast<std::uint32_t>(
                    (i - 1) * 2
                );
                const auto b = a + 2;
                mesh.indices.insert(mesh.indices.end(), {
                    a, a + 1, b,
                    b, a + 1, b + 1
                });
            }
        }

        void append_lake(
            engine::renderer::SurfaceMesh& mesh,
            const LakeArea& lake
        ) {
            constexpr std::uint32_t sectors = 48;
            const auto start = static_cast<std::uint32_t>(mesh.vertices.size());
            const float y = lake.center.y + 0.55f;

            mesh.vertices.push_back({
                lake.center.x,
                y,
                lake.center.z,
                lake.center.x / textureScale,
                lake.center.z / textureScale
            });

            for (std::uint32_t i = 0; i <= sectors; ++i) {
                const float angle = static_cast<float>(i) *
                    2.0f * std::numbers::pi_v<float> /
                    static_cast<float>(sectors);
                const float x = lake.center.x +
                    std::cos(angle) * lake.radiusMeters;
                const float z = lake.center.z +
                    std::sin(angle) * lake.radiusMeters;

                mesh.vertices.push_back({
                    x, y, z,
                    x / textureScale,
                    z / textureScale
                });

                if (i > 0) {
                    mesh.indices.insert(mesh.indices.end(), {
                        start,
                        start + i,
                        start + i + 1
                    });
                }
            }
        }

        void append_marsh(
            engine::renderer::SurfaceMesh& mesh,
            const HydrologyLayer& water,
            const terrain::Heightfield& terrain
        ) {
            constexpr std::uint32_t sectors = 12;
            const float radius = std::min(
                water.spacingMeters * 0.45f,
                4.0f
            );

            for (const WaterPoint& marker : water.marshMarkers) {
                const auto start = static_cast<std::uint32_t>(
                    mesh.vertices.size()
                );
                mesh.vertices.push_back({
                    marker.x,
                    terrain.sample_height(marker.x, marker.z) + 0.16f,
                    marker.z,
                    marker.x / textureScale,
                    marker.z / textureScale
                });

                for (std::uint32_t i = 0; i <= sectors; ++i) {
                    const float angle = static_cast<float>(i) *
                        2.0f * std::numbers::pi_v<float> /
                        static_cast<float>(sectors);
                    const float x = marker.x + std::cos(angle) * radius;
                    const float z = marker.z + std::sin(angle) * radius;
                    mesh.vertices.push_back({
                        x,
                        terrain.sample_height(x, z) + 0.16f,
                        z,
                        x / textureScale,
                        z / textureScale
                    });

                    if (i > 0) {
                        mesh.indices.insert(mesh.indices.end(), {
                            start, start + i, start + i + 1
                        });
                    }
                }
            }
        }

    }

    WaterSurfaces build_water_surfaces(
        const HydrologyLayer& water,
        const terrain::Heightfield& terrain
    ) {
        if (!water.valid() || !terrain.valid() ||
            water.samplesX != terrain.samplesX ||
            water.samplesZ != terrain.samplesZ ||
            water.originX != terrain.originX ||
            water.originZ != terrain.originZ ||
            water.spacingMeters != terrain.spacingMeters) {
            throw std::invalid_argument(
                "Water surfaces require matching Hydrology and Terrain"
            );
        }

        WaterSurfaces surfaces;
        append_channel(
            surfaces.openWater,
            water.river,
            water.settings.riverWidthMeters
        );

        for (const auto& channel : water.tributaries) {
            append_channel(
                surfaces.openWater,
                channel,
                water.settings.riverWidthMeters
            );
        }

        for (const LakeArea& lake : water.lakes) {
            append_lake(surfaces.openWater, lake);
        }

        append_marsh(surfaces.marsh, water, terrain);
        return surfaces;
    }

}
