#pragma once

#include "Core/Math/Vector3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::assets
{
    enum class MeshIndexFormat : std::uint8_t
    {
        UInt16 = 0,
        UInt32 = 1
    };

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

        bool renderEnabled =
            true;
    };

    struct MeshData final
    {
        std::string vertexFormat;

        bool skinned =
            false;

        MeshIndexFormat indexFormat =
            MeshIndexFormat::UInt16;

        std::vector<MeshVertex>
            vertices;

        std::vector<std::uint16_t>
            indices;

        std::vector<std::uint32_t>
            indices32;

        std::vector<MeshPrimitiveGroup>
            primitiveGroups;

        [[nodiscard]]
        std::size_t IndexCount() const noexcept
        {
            if (indexFormat ==
                MeshIndexFormat::UInt32)
            {
                return
                    indices32.size();
            }

            return
                indices.size();
        }

        [[nodiscard]]
        bool HasIndices() const noexcept
        {
            return
                IndexCount() !=
                0;
        }

        [[nodiscard]]
        std::size_t IndexElementSize() const noexcept
        {
            if (indexFormat ==
                MeshIndexFormat::UInt32)
            {
                return
                    sizeof(std::uint32_t);
            }

            return
                sizeof(std::uint16_t);
        }

        [[nodiscard]]
        const void* IndexData() const noexcept
        {
            if (indexFormat ==
                MeshIndexFormat::UInt32)
            {
                if (indices32.empty())
                {
                    return nullptr;
                }

                return
                    indices32.data();
            }

            if (indices.empty())
            {
                return nullptr;
            }

            return
                indices.data();
        }

        [[nodiscard]]
        std::uint32_t IndexAt(
            const std::size_t index) const noexcept
        {
            if (indexFormat ==
                MeshIndexFormat::UInt32)
            {
                return
                    indices32[index];
            }

            return
                indices[index];
        }

        [[nodiscard]]
        std::size_t TriangleCount() const noexcept
        {
            return
                IndexCount() /
                3;
        }
    };
}
