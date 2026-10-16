
#include "Engine/Renderer/TerrainChunkMesh.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace engine::renderer {

namespace {

std::vector<std::uint32_t> make_coordinates(
    std::uint32_t start,
    std::uint32_t end,
    std::uint32_t stride
) {
    std::vector<std::uint32_t> result;

    for (std::uint32_t i = start; i < end; i += stride) {
        result.push_back(i);
    }

    result.push_back(end);
    return result;
}

void append_skirt(
    TerrainChunkGeometry& geometry,
    const std::vector<std::uint32_t>& edge,
    float depth
) {
    if (edge.size() < 2) {
        return;
    }

    const auto base = static_cast<std::uint32_t>(
        geometry.vertices.size()
    );

    for (const auto index : edge) {
        TerrainVertex vertex = geometry.vertices[index];
        vertex.y -= depth;
        geometry.vertices.push_back(vertex);
    }

    for (std::uint32_t i = 0;
         i + 1 < edge.size();
         ++i) {

        const std::uint32_t a = edge[i];
        const std::uint32_t b = edge[i + 1];

        const std::uint32_t c = base + i;
        const std::uint32_t d = base + i + 1;

        geometry.indices.insert(
            geometry.indices.end(),
            {a, c, b, b, c, d}
        );
    }
}

}

TerrainChunkGeometry build_chunk_geometry(
    const TerrainMeshData& data,
    std::uint32_t startX,
    std::uint32_t startZ,
    std::uint32_t stride,
    float skirtDepth
) {
    if (data.samplesX < 2 ||
        data.samplesZ < 2 ||
        stride == 0 ||
        startX >= data.samplesX - 1 ||
        startZ >= data.samplesZ - 1 ||
        data.heights.size() !=
            static_cast<std::size_t>(data.samplesX) *
            data.samplesZ) {
        throw std::invalid_argument(
            "Invalid terrain chunk parameters"
        );
    }

    const auto endX = (std::min)(
        startX + TerrainChunkCells,
        data.samplesX - 1
    );

    const auto endZ = (std::min)(
        startZ + TerrainChunkCells,
        data.samplesZ - 1
    );

    const auto xs = make_coordinates(
        startX, endX, stride
    );

    const auto zs = make_coordinates(
        startZ, endZ, stride
    );

    const std::uint32_t columns =
        static_cast<std::uint32_t>(xs.size());

    const std::uint32_t rows =
        static_cast<std::uint32_t>(zs.size());

    const auto height = [&](std::uint32_t x,
                            std::uint32_t z) -> float {
        return data.heights[
            static_cast<std::size_t>(z) * data.samplesX + x
        ];
    };

    TerrainChunkGeometry result;

    result.centerX = data.originX +
        (static_cast<float>(startX + endX) * 0.5f) *
        data.spacingMeters;

    result.centerZ = data.originZ +
        (static_cast<float>(startZ + endZ) * 0.5f) *
        data.spacingMeters;

    result.vertices.reserve(
        static_cast<std::size_t>(columns) * rows +
        2 * (columns + rows)
    );

    result.indices.reserve(
        static_cast<std::size_t>(columns - 1) *
        (rows - 1) * 6 +
        12 * (columns + rows)
    );

    for (const auto z : zs) {
        for (const auto x : xs) {
            const auto x0 = x > 0 ? x - 1 : x;
            const auto z0 = z > 0 ? z - 1 : z;

            const auto x1 = (std::min)(
                x + 1, data.samplesX - 1
            );

            const auto z1 = (std::min)(
                z + 1, data.samplesZ - 1
            );

            const float dx =
                (height(x1, z) - height(x0, z)) /
                ((x1 - x0) * data.spacingMeters);

            const float dz =
                (height(x, z1) - height(x, z0)) /
                ((z1 - z0) * data.spacingMeters);

            const float length = std::sqrt(
                dx * dx + dz * dz + 1.0f
            );

            TerrainVertex vertex{
                data.originX +
                    static_cast<float>(x) * data.spacingMeters,
                height(x, z),
                data.originZ +
                    static_cast<float>(z) * data.spacingMeters,
                -dx / length,
                1.0f / length,
                -dz / length
            };

            if (!std::isfinite(vertex.x) ||
                !std::isfinite(vertex.y) ||
                !std::isfinite(vertex.z) ||
                !std::isfinite(vertex.nx) ||
                !std::isfinite(vertex.ny) ||
                !std::isfinite(vertex.nz)) {
                throw std::invalid_argument(
                    "Terrain contains invalid vertex data"
                );
            }

            result.vertices.push_back(vertex);
        }
    }

    for (std::uint32_t z = 0; z + 1 < rows; ++z) {
        for (std::uint32_t x = 0; x + 1 < columns; ++x) {
            const std::uint32_t a = z * columns + x;
            const std::uint32_t b = a + 1;
            const std::uint32_t c = a + columns;
            const std::uint32_t d = c + 1;

            result.indices.insert(
                result.indices.end(),
                {a, c, b, b, c, d}
            );
        }
    }

    std::vector<std::uint32_t> edge;
    edge.reserve((std::max)(columns, rows));

    // North edge.
    if (startZ > 0) {
        edge.clear();

        for (std::uint32_t x = 0; x < columns; ++x) {
            edge.push_back(x);
        }

        append_skirt(result, edge, skirtDepth);
    }

    // South edge.
    if (endZ < data.samplesZ - 1) {
        edge.clear();

        for (std::uint32_t x = 0; x < columns; ++x) {
            edge.push_back((rows - 1) * columns + x);
        }

        append_skirt(result, edge, skirtDepth);
    }

    // West edge.
    if (startX > 0) {
        edge.clear();

        for (std::uint32_t z = 0; z < rows; ++z) {
            edge.push_back(z * columns);
        }

        append_skirt(result, edge, skirtDepth);
    }

    // East edge.
    if (endX < data.samplesX - 1) {
        edge.clear();

        for (std::uint32_t z = 0; z < rows; ++z) {
            edge.push_back(z * columns + columns - 1);
        }

        append_skirt(result, edge, skirtDepth);
    }

    return result;
}

}
