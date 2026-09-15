#include "Core/World/Water/WaterMeshBuilder.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace
{
    constexpr std::uint32_t MaximumWaterSegments =
        254;

    constexpr std::uint32_t PackedUpNormal =
        0x001FF800u;

    bool IsFinite(
        const core::math::Vector3& value) noexcept
    {
        return
            std::isfinite(value.x) &&
            std::isfinite(value.y) &&
            std::isfinite(value.z);
    }

    std::uint32_t CalculateSegmentCount(
        const float size,
        const float cellSize) noexcept
    {
        const float raw =
            std::ceil(
                size /
                cellSize);

        if (raw <= 1.0f)
        {
            return 1;
        }

        if (raw >=
            static_cast<float>(
                MaximumWaterSegments))
        {
            return
                MaximumWaterSegments;
        }

        return
            static_cast<std::uint32_t>(
                raw);
    }

    core::math::Transform3x4 BuildTransform(
        const core::world::water::WaterDefinition& definition) noexcept
    {
        const float cosine =
            std::cos(
                definition.orientation);

        const float sine =
            std::sin(
                definition.orientation);

        core::math::Transform3x4
            transform;

        transform.values =
        {
            cosine,
            0.0f,
            -sine,

            0.0f,
            1.0f,
            0.0f,

            sine,
            0.0f,
            cosine,

            definition.position.x,
            definition.position.y,
            definition.position.z
        };

        return transform;
    }
}

namespace core::world::water
{
    bool WaterMeshBuilder::Build(
        const WaterDefinition& definition,
        assets::MeshData& outputMesh,
        math::Transform3x4& outputTransform,
        std::string& error) const
    {
        outputMesh = {};
        outputTransform =
            math::Transform3x4::Identity();

        error.clear();

        if (!IsFinite(
                definition.position))
        {
            error =
                "Water position contains invalid values.";

            return false;
        }

        if (!IsFinite(
                definition.size))
        {
            error =
                "Water size contains invalid values.";

            return false;
        }

        if (!std::isfinite(
                definition.orientation))
        {
            error =
                "Water orientation is invalid.";

            return false;
        }

        if (definition.size.x <= 0.0f ||
            definition.size.z <= 0.0f)
        {
            error =
                "Water surface dimensions are invalid.";

            return false;
        }

        if (!std::isfinite(
                definition.cellSize) ||
            definition.cellSize <= 0.0f)
        {
            error =
                "Water cell size is invalid.";

            return false;
        }

        const std::uint32_t segmentCountX =
            CalculateSegmentCount(
                definition.size.x,
                definition.cellSize);

        const std::uint32_t segmentCountZ =
            CalculateSegmentCount(
                definition.size.z,
                definition.cellSize);

        const std::uint32_t vertexCountX =
            segmentCountX +
            1;

        const std::uint32_t vertexCountZ =
            segmentCountZ +
            1;

        const std::uint64_t vertexCount =
            static_cast<std::uint64_t>(
                vertexCountX) *
            static_cast<std::uint64_t>(
                vertexCountZ);

        if (vertexCount >
            static_cast<std::uint64_t>(
                std::numeric_limits<
                    std::uint16_t>::max()))
        {
            error =
                "Water mesh requires 32-bit indices.";

            return false;
        }

        assets::MeshData
            mesh;

        mesh.vertexFormat =
            "water-vlo";

        mesh.vertices.resize(
            static_cast<std::size_t>(
                vertexCount));

        const float halfSizeX =
            definition.size.x *
            0.5f;

        const float halfSizeZ =
            definition.size.z *
            0.5f;

        for (std::uint32_t z = 0;
             z < vertexCountZ;
             ++z)
        {
            const float normalizedZ =
                static_cast<float>(z) /
                static_cast<float>(
                    segmentCountZ);

            const float localZ =
                -halfSizeZ +
                normalizedZ *
                    definition.size.z;

            for (std::uint32_t x = 0;
                 x < vertexCountX;
                 ++x)
            {
                const float normalizedX =
                    static_cast<float>(x) /
                    static_cast<float>(
                        segmentCountX);

                const float localX =
                    -halfSizeX +
                    normalizedX *
                        definition.size.x;

                assets::MeshVertex
                    vertex;

                vertex.position =
                {
                    localX,
                    0.0f,
                    localZ
                };

                vertex.packedNormal =
                    PackedUpNormal;

                vertex.u =
                    normalizedX;

                vertex.v =
                    normalizedZ;

                vertex.colour =
                    0xFFFFFFFFu;

                mesh.vertices[
                    static_cast<std::size_t>(
                        z) *
                        vertexCountX +
                    x] =
                        vertex;
            }
        }

        mesh.indices.reserve(
            static_cast<std::size_t>(
                segmentCountX) *
            static_cast<std::size_t>(
                segmentCountZ) *
            6);

        const auto index =
            [vertexCountX](
                const std::uint32_t x,
                const std::uint32_t z)
            {
                return
                    static_cast<std::uint16_t>(
                        z *
                            vertexCountX +
                        x);
            };

        for (std::uint32_t z = 0;
             z < segmentCountZ;
             ++z)
        {
            for (std::uint32_t x = 0;
                 x < segmentCountX;
                 ++x)
            {
                const std::uint16_t i00 =
                    index(
                        x,
                        z);

                const std::uint16_t i01 =
                    index(
                        x,
                        z + 1);

                const std::uint16_t i10 =
                    index(
                        x + 1,
                        z);

                const std::uint16_t i11 =
                    index(
                        x + 1,
                        z + 1);

                mesh.indices.push_back(
                    i00);

                mesh.indices.push_back(
                    i01);

                mesh.indices.push_back(
                    i10);

                mesh.indices.push_back(
                    i10);

                mesh.indices.push_back(
                    i01);

                mesh.indices.push_back(
                    i11);
            }
        }

        assets::MeshPrimitiveGroup
            primitiveGroup;

        primitiveGroup.startIndex =
            0;

        primitiveGroup.primitiveCount =
            static_cast<std::uint32_t>(
                mesh.indices.size() /
                3);

        primitiveGroup.startVertex =
            0;

        primitiveGroup.vertexCount =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        mesh.primitiveGroups.push_back(
            primitiveGroup);

        outputTransform =
            BuildTransform(
                definition);

        outputMesh =
            std::move(
                mesh);

        return true;
    }
}