#pragma once

#include "Core/Math/Vector3.h"

#include <array>
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

        //
        // BigWorld skinned vertex:
        //
        // IIIWW =
        //
        // index1
        // index2
        // index3
        // weight1
        // weight2
        //
        // weight3 = 1 - w1 - w2
        //
        std::array<
            std::uint16_t,
            3>
            boneIndices
        {
            0,
            0,
            0
        };

        std::array<
            float,
            3>
            boneWeights
        {
            1.0f,
            0.0f,
            0.0f
        };
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

        bool skinned =
            false;

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