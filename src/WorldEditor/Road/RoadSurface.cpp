#include "WorldEditor/Road/RoadSurface.h"

#include "Engine/Assets/PrimitivesMeshLoader.h"
#include "WorldEditor/Road/RoadCrossings.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>

namespace world_editor::road {

    namespace {

        struct PathPoint {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            float distance = 0.0f;
            float sideX = 0.0f;
            float sideZ = 1.0f;
            float width = 8.0f;
        };

        [[nodiscard]] bool deep_water(biome::WaterKind kind) noexcept {
            return kind == biome::WaterKind::River ||
                kind == biome::WaterKind::Lake;
        }

        [[nodiscard]] std::vector<PathPoint> make_path(
            const std::vector<RoadRoutePoint>& route,
            std::size_t first,
            std::size_t last,
            float roadWidth,
            float startWidth,
            float endWidth
        ) {
            std::vector<PathPoint> path;
            path.reserve(last - first);

            for (std::size_t i = first; i < last; ++i) {
                const auto& p = route[i];
                PathPoint point{p.x, p.y, p.z};

                if (i > first && i + 1 < last) {
                    const auto& a = route[i - 1];
                    const auto& b = route[i + 1];
                    point.x = (a.x + 2.0f * p.x + b.x) * 0.25f;
                    point.y = (a.y + 2.0f * p.y + b.y) * 0.25f;
                    point.z = (a.z + 2.0f * p.z + b.z) * 0.25f;
                }

                if (!path.empty()) {
                    point.distance = path.back().distance + std::hypot(
                        point.x - path.back().x,
                        point.z - path.back().z
                    );
                }

                path.push_back(point);
            }

            for (std::size_t i = 0; i < path.size(); ++i) {
                const auto& a = path[i > 0 ? i - 1 : i];
                const auto& p = path[i];
                const auto& b = path[i + 1 < path.size() ? i + 1 : i];
                const float inLength = std::hypot(p.x - a.x, p.z - a.z);
                const float outLength = std::hypot(b.x - p.x, b.z - p.z);
                constexpr float transitionMeters = 24.0f;
                const float fromStart = std::clamp(
                    p.distance / transitionMeters, 0.0f, 1.0f
                );
                const float fromEnd = std::clamp(
                    (path.back().distance - p.distance) /
                        transitionMeters,
                    0.0f,
                    1.0f
                );
                path[i].width = std::min({
                    roadWidth,
                    std::lerp(startWidth, roadWidth, fromStart),
                    std::lerp(endWidth, roadWidth, fromEnd)
                });

                if (inLength > 0.001f && outLength > 0.001f) {
                    const float inX = -(p.z - a.z) / inLength;
                    const float inZ = (p.x - a.x) / inLength;
                    const float outX = -(b.z - p.z) / outLength;
                    const float outZ = (b.x - p.x) / outLength;
                    const float joinX = inX + outX;
                    const float joinZ = inZ + outZ;
                    const float joinLength = std::hypot(joinX, joinZ);

                    if (joinLength > 0.001f) {
                        const float projection = (
                            joinX * outX + joinZ * outZ
                        ) / joinLength;
                        const float scale = std::min(
                            1.35f,
                            1.0f / std::max(0.6f, projection)
                        );
                        path[i].sideX = joinX / joinLength * scale;
                        path[i].sideZ = joinZ / joinLength * scale;
                    }
                } else if (outLength > 0.001f) {
                    path[i].sideX = -(b.z - p.z) / outLength;
                    path[i].sideZ = (b.x - p.x) / outLength;
                } else if (inLength > 0.001f) {
                    path[i].sideX = -(p.z - a.z) / inLength;
                    path[i].sideZ = (p.x - a.x) / inLength;
                }
            }

            return path;
        }

        [[nodiscard]] PathPoint sample_path(
            const std::vector<PathPoint>& path,
            float distance
        ) {
            const auto found = std::lower_bound(
                path.begin() + 1,
                path.end(),
                distance,
                [](const PathPoint& point, float value) {
                    return point.distance < value;
                }
            );
            const auto next = found == path.end()
                ? path.end() - 1
                : found;
            const auto previous = next - 1;
            const float length = next->distance - previous->distance;
            const float t = length > 0.001f
                ? std::clamp(
                    (distance - previous->distance) / length,
                    0.0f,
                    1.0f
                )
                : 0.0f;

            PathPoint result;
            result.x = std::lerp(previous->x, next->x, t);
            result.y = std::lerp(previous->y, next->y, t);
            result.z = std::lerp(previous->z, next->z, t);
            result.sideX = std::lerp(previous->sideX, next->sideX, t);
            result.sideZ = std::lerp(previous->sideZ, next->sideZ, t);
            result.width = std::lerp(previous->width, next->width, t);
            return result;
        }

        void append_path(
            engine::renderer::SurfaceMesh& output,
            const std::vector<PathPoint>& path,
            const engine::assets::PrimitivesMesh& model,
            float modelLength,
            float modelWidth,
            float modelMinX,
            float modelUvSpan
        ) {
            if (path.size() < 2 || path.back().distance < 0.1f) {
                return;
            }

            float start = 0.0f;

            while (start < path.back().distance) {
                const float length = std::min(
                    modelLength,
                    path.back().distance - start
                );
                const auto base = static_cast<std::uint32_t>(
                    output.vertices.size()
                );

                for (const auto& vertex : model.vertices) {
                    const float fraction = std::clamp(
                        (vertex.x - modelMinX) / modelLength,
                        0.0f,
                        1.0f
                    );
                    const PathPoint point = sample_path(
                        path, start + fraction * length
                    );
                    const float side = vertex.z * point.width / modelWidth;

                    output.vertices.push_back({
                        point.x + point.sideX * side,
                        point.y + vertex.y + 0.18f,
                        point.z + point.sideZ * side,
                        vertex.u + start / modelLength * modelUvSpan,
                        vertex.v
                    });
                }

                for (std::uint32_t index : model.indices) {
                    output.indices.push_back(base + index);
                }

                start += modelLength;
            }
        }

    }

    engine::renderer::SurfaceMesh build_road_surface(
        const RoadGraph& graph,
        const std::filesystem::path& segmentModel,
        float widthMeters
    ) {
        if (!std::isfinite(widthMeters) || widthMeters <= 0.0f) {
            throw std::invalid_argument("Invalid road width");
        }

        const auto model = engine::assets::load_primitives_mesh(segmentModel);
        float minX = (std::numeric_limits<float>::max)();
        float maxX = (std::numeric_limits<float>::lowest)();
        float minZ = (std::numeric_limits<float>::max)();
        float maxZ = (std::numeric_limits<float>::lowest)();
        float minU = (std::numeric_limits<float>::max)();
        float maxU = (std::numeric_limits<float>::lowest)();

        for (const auto& vertex : model.vertices) {
            minX = std::min(minX, vertex.x);
            maxX = std::max(maxX, vertex.x);
            minZ = std::min(minZ, vertex.z);
            maxZ = std::max(maxZ, vertex.z);
            minU = std::min(minU, vertex.u);
            maxU = std::max(maxU, vertex.u);
        }

        const float modelLength = maxX - minX;
        const float modelWidth = maxZ - minZ;

        if (modelLength < 0.25f || modelWidth < 1.0f) {
            throw std::runtime_error("Invalid road segment dimensions");
        }

        engine::renderer::SurfaceMesh output;

        const auto crossings = find_crossings(graph);

        for (std::size_t edgeIndex = 0;
             edgeIndex < graph.edges.size();
             ++edgeIndex) {
            const RoadEdge& edge = graph.edges[edgeIndex];
            const auto& route = edge.route;
            std::vector<float> anchorWidths(route.size(), widthMeters);
            std::vector<float> anchorSideX(route.size(), 0.0f);
            std::vector<float> anchorSideZ(route.size(), 1.0f);
            std::vector<bool> bridgeAnchor(route.size(), false);

            for (const auto& crossing : crossings) {
                if (crossing.edgeIndex == edgeIndex) {
                    const float deckWidth =
                        crossing_deck_width(crossing.kind);
                    anchorWidths[crossing.firstPoint] = deckWidth;
                    anchorWidths[crossing.lastPoint] = deckWidth;

                    const auto& first = route[crossing.firstPoint];
                    const auto& last = route[crossing.lastPoint];
                    const float dx = last.x - first.x;
                    const float dz = last.z - first.z;
                    const float length = std::hypot(dx, dz);

                    if (length > 0.001f) {
                        for (const std::size_t anchor : {
                                 crossing.firstPoint,
                                 crossing.lastPoint
                             }) {
                            anchorSideX[anchor] = -dz / length;
                            anchorSideZ[anchor] = dx / length;
                            bridgeAnchor[anchor] = true;
                        }
                    }
                }
            }

            for (std::size_t first = 0; first < route.size();) {
                while (first < route.size() &&
                    deep_water(route[first].water)) {
                    ++first;
                }

                std::size_t last = first;

                while (last < route.size() &&
                    !deep_water(route[last].water)) {
                    ++last;
                }

                if (last > first + 1) {
                    auto path = make_path(
                        route,
                        first,
                        last,
                        widthMeters,
                        anchorWidths[first],
                        anchorWidths[last - 1]
                    );

                    if (bridgeAnchor[first]) {
                        path.front().sideX = anchorSideX[first];
                        path.front().sideZ = anchorSideZ[first];
                    }

                    if (bridgeAnchor[last - 1]) {
                        path.back().sideX = anchorSideX[last - 1];
                        path.back().sideZ = anchorSideZ[last - 1];
                    }

                    append_path(
                        output,
                        path,
                        model,
                        modelLength,
                        modelWidth,
                        minX,
                        maxU - minU
                    );
                }

                first = last;
            }
        }

        return output;
    }

}
