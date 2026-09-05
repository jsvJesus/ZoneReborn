#include "Core/World/TerrainMeshBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace
{
    constexpr float TerrainBlockSize =
        100.0f;

    core::math::Vector3 Normalize(
        const core::math::Vector3 value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;

        if (lengthSquared <=
            0.000001f)
        {
            return
            {
                0.0f,
                1.0f,
                0.0f
            };
        }

        const float inverseLength =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
        {
            value.x *
                inverseLength,

            value.y *
                inverseLength,

            value.z *
                inverseLength
        };
    }

    std::uint32_t PackNormal(
        const core::math::Vector3 value) noexcept
    {
        const core::math::Vector3 normal =
            Normalize(
                value);

        const std::int32_t x =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.x,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t y =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.y,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t z =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.z,
                    -1.0f,
                    1.0f) *
                511.0f);

        return
            (
                static_cast<std::uint32_t>(
                    x) &
                0x7FFu
            ) |

            (
                (
                    static_cast<std::uint32_t>(
                        y) &
                    0x7FFu
                )
                << 11u
            ) |

            (
                (
                    static_cast<std::uint32_t>(
                        z) &
                    0x3FFu
                )
                << 22u
            );
    }

    bool IsHoleCell(
        const core::world::TerrainHoleData& holes,
        const std::uint32_t terrainX,
        const std::uint32_t terrainZ,
        const std::uint32_t terrainCellWidth,
        const std::uint32_t terrainCellHeight) noexcept
    {
        if (!holes.present ||
            holes.width == 0 ||
            holes.height == 0 ||
            terrainCellWidth == 0 ||
            terrainCellHeight == 0)
        {
            return false;
        }

        const float normalizedX =
            (
                static_cast<float>(
                    terrainX) +
                0.5f
            ) /
            static_cast<float>(
                terrainCellWidth);

        const float normalizedZ =
            (
                static_cast<float>(
                    terrainZ) +
                0.5f
            ) /
            static_cast<float>(
                terrainCellHeight);

        const std::uint32_t holeX =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedX *
                    static_cast<float>(
                        holes.width)),
                holes.width -
                    1);

        const std::uint32_t holeZ =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedZ *
                    static_cast<float>(
                        holes.height)),
                holes.height -
                    1);

        return
            holes.IsHole(
                holeX,
                holeZ);
    }
}

namespace core::world
{
    bool TerrainMeshBuilder::Build(
        const TerrainHeightData& heightData,
        const TerrainHoleData& holeData,
        assets::MeshData& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        const std::uint32_t width =
            heightData.VisibleWidth();

        const std::uint32_t height =
            heightData.VisibleHeight();

        if (width < 2 ||
            height < 2)
        {
            error =
                "Visible terrain height map is invalid.";

            return false;
        }

        const std::uint64_t vertexCount =
            static_cast<std::uint64_t>(
                width) *
            height;

        if (vertexCount >
            std::numeric_limits<std::uint16_t>::max())
        {
            error =
                "Terrain requires 32-bit indices.";

            return false;
        }

        const float spacingX =
            TerrainBlockSize /
            static_cast<float>(
                width -
                1);

        const float spacingZ =
            TerrainBlockSize /
            static_cast<float>(
                height -
                1);

        const auto visibleHeight =
            [&heightData](
                const std::uint32_t x,
                const std::uint32_t z)
            {
                return heightData.At(
                    x +
                        heightData.visibleOffset,
                    z +
                        heightData.visibleOffset);
            };

        assets::MeshData mesh;

        mesh.vertexFormat =
            "terrain2-heightmap";

        mesh.vertices.resize(
            static_cast<std::size_t>(
                vertexCount));

        for (std::uint32_t z = 0;
             z < height;
             ++z)
        {
            for (std::uint32_t x = 0;
                 x < width;
                 ++x)
            {
                const std::uint32_t sourceX =
                    x +
                    heightData.visibleOffset;

                const std::uint32_t sourceZ =
                    z +
                    heightData.visibleOffset;

                const std::uint32_t leftX =
                    sourceX >
                        0
                        ? sourceX -
                            1
                        : sourceX;

                const std::uint32_t rightX =
                    sourceX +
                            1 <
                        heightData.width
                        ? sourceX +
                            1
                        : sourceX;

                const std::uint32_t downZ =
                    sourceZ >
                        0
                        ? sourceZ -
                            1
                        : sourceZ;

                const std::uint32_t upZ =
                    sourceZ +
                            1 <
                        heightData.height
                        ? sourceZ +
                            1
                        : sourceZ;

                const float leftHeight =
                    heightData.At(
                        leftX,
                        sourceZ);

                const float rightHeight =
                    heightData.At(
                        rightX,
                        sourceZ);

                const float downHeight =
                    heightData.At(
                        sourceX,
                        downZ);

                const float upHeight =
                    heightData.At(
                        sourceX,
                        upZ);

                const float distanceX =
                    static_cast<float>(
                        rightX -
                        leftX) *
                    spacingX;

                const float distanceZ =
                    static_cast<float>(
                        upZ -
                        downZ) *
                    spacingZ;

                const float slopeX =
                    distanceX >
                            0.0f
                        ? (
                            rightHeight -
                            leftHeight
                        ) /
                            distanceX
                        : 0.0f;

                const float slopeZ =
                    distanceZ >
                            0.0f
                        ? (
                            upHeight -
                            downHeight
                        ) /
                            distanceZ
                        : 0.0f;

                assets::MeshVertex vertex;

                vertex.position =
                {
                    static_cast<float>(
                        x) *
                        spacingX,

                    visibleHeight(
                        x,
                        z),

                    static_cast<float>(
                        z) *
                        spacingZ
                };

                vertex.packedNormal =
                    PackNormal(
                    {
                        -slopeX,
                        1.0f,
                        -slopeZ
                    });

                vertex.u =
                    static_cast<float>(
                        x) /
                    static_cast<float>(
                        width -
                        1);

                vertex.v =
                    static_cast<float>(
                        z) /
                    static_cast<float>(
                        height -
                        1);

                vertex.colour =
                    0xFFFFFFFFu;

                mesh.vertices[
                    static_cast<std::size_t>(
                        z) *
                        width +
                    x] =
                    vertex;
            }
        }

        const std::uint32_t terrainCellWidth =
            width -
            1;

        const std::uint32_t terrainCellHeight =
            height -
            1;

        mesh.indices.reserve(
            static_cast<std::size_t>(
                terrainCellWidth) *
            terrainCellHeight *
            6);

        const auto index =
            [width](
                const std::uint32_t x,
                const std::uint32_t z)
            {
                return
                    static_cast<std::uint16_t>(
                        z *
                            width +
                        x);
            };

        for (std::uint32_t z = 0;
             z <
                terrainCellHeight;
             ++z)
        {
            for (std::uint32_t x = 0;
                 x <
                    terrainCellWidth;
                 ++x)
            {
                if (IsHoleCell(
                        holeData,
                        x,
                        z,
                        terrainCellWidth,
                        terrainCellHeight))
                {
                    continue;
                }

                const std::uint16_t i00 =
                    index(
                        x,
                        z);

                const std::uint16_t i01 =
                    index(
                        x,
                        z +
                            1);

                const std::uint16_t i10 =
                    index(
                        x +
                            1,
                        z);

                const std::uint16_t i11 =
                    index(
                        x +
                            1,
                        z +
                            1);

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
            group;

        group.startIndex =
            0;

        group.primitiveCount =
            static_cast<std::uint32_t>(
                mesh.indices.size() /
                3);

        group.startVertex =
            0;

        group.vertexCount =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        mesh.primitiveGroups.push_back(
            group);

        output =
            std::move(
                mesh);

        return true;
    }
}