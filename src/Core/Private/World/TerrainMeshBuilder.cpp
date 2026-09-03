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
                static_cast<std::uint32_t>(x) &
                0x7FFu
            ) |

            (
                (
                    static_cast<std::uint32_t>(y) &
                    0x7FFu
                )
                << 11u
            ) |

            (
                (
                    static_cast<std::uint32_t>(z) &
                    0x3FFu
                )
                << 22u
            );
    }
}

namespace core::world
{
    bool TerrainMeshBuilder::Build(
        const TerrainHeightData& heightData,
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
                width - 1);

        const float spacingZ =
            TerrainBlockSize /
            static_cast<float>(
                height - 1);

        const auto sample =
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
                const std::uint32_t leftX =
                    x == 0
                        ? 0
                        : x - 1;

                const std::uint32_t rightX =
                    x + 1 < width
                        ? x + 1
                        : x;

                const std::uint32_t downZ =
                    z == 0
                        ? 0
                        : z - 1;

                const std::uint32_t upZ =
                    z + 1 < height
                        ? z + 1
                        : z;

                const float leftHeight =
                    sample(
                        leftX,
                        z);

                const float rightHeight =
                    sample(
                        rightX,
                        z);

                const float downHeight =
                    sample(
                        x,
                        downZ);

                const float upHeight =
                    sample(
                        x,
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
                    distanceX > 0.0f
                        ? (
                            rightHeight -
                            leftHeight
                        ) /
                            distanceX
                        : 0.0f;

                const float slopeZ =
                    distanceZ > 0.0f
                        ? (
                            upHeight -
                            downHeight
                        ) /
                            distanceZ
                        : 0.0f;

                assets::MeshVertex vertex;

                vertex.position =
                {
                    static_cast<float>(x) *
                        spacingX,

                    sample(
                        x,
                        z),

                    static_cast<float>(z) *
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
                    static_cast<float>(x) /
                    static_cast<float>(
                        width - 1);

                vertex.v =
                    static_cast<float>(z) /
                    static_cast<float>(
                        height - 1);

                vertex.colour =
                    0xFFFFFFFFu;

                mesh.vertices[
                    static_cast<std::size_t>(z) *
                        width +
                    x] =
                    vertex;
            }
        }

        mesh.indices.reserve(
            static_cast<std::size_t>(
                width - 1) *
            static_cast<std::size_t>(
                height - 1) *
            6);

        const auto index =
            [width](
                const std::uint32_t x,
                const std::uint32_t z)
            {
                return
                    static_cast<std::uint16_t>(
                        z * width +
                        x);
            };

        for (std::uint32_t z = 0;
             z + 1 < height;
             ++z)
        {
            for (std::uint32_t x = 0;
                 x + 1 < width;
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

        assets::MeshPrimitiveGroup group;

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
            std::move(mesh);

        return true;
    }
}