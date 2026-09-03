#pragma once

#include "Core/Math/Vector3.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::assets
{
    struct MeshVertex final
    {
        math::Vector3 position;

        std::uint32_t packedNormal = 0;

        float u = 0.0f;
        float v = 0.0f;

        float u2 = 0.0f;
        float v2 = 0.0f;

        std::uint32_t packedTangent = 0;
        std::uint32_t packedBinormal = 0;

        std::uint32_t colour =
            0xFFFFFFFFu;
    };

    struct MeshPrimitiveGroup final
    {
        std::uint32_t startIndex = 0;
        std::uint32_t primitiveCount = 0;

        std::uint32_t startVertex = 0;
        std::uint32_t vertexCount = 0;
    };

    struct MeshData final
    {
        std::string vertexFormat;

        std::vector<MeshVertex>
            vertices;

        std::vector<std::uint16_t>
            indices;

        std::vector<MeshPrimitiveGroup>
            primitiveGroups;

        [[nodiscard]]
        std::size_t TriangleCount() const noexcept
        {
            return
                indices.size() /
                3;
        }
    };
}