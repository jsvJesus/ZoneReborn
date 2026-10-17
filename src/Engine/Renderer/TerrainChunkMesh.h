
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace engine::renderer {

    inline constexpr std::uint32_t TerrainChunkCells = 64;
    inline constexpr std::uint32_t TerrainLodCount = 4;

    struct TerrainMeshData {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 1.0f;

        std::span<const float> heights;
    };

    struct TerrainVertex {
        float x, y, z;
        float nx, ny, nz;
    };

    struct TerrainChunkGeometry {
        float centerX = 0.0f;
        float centerZ = 0.0f;

        std::vector<TerrainVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    [[nodiscard]] TerrainChunkGeometry build_chunk_geometry(
        const TerrainMeshData& data,
        std::uint32_t startX,
        std::uint32_t startZ,
        std::uint32_t stride,
        float skirtDepth,
        bool allBorders = false
   );

}
