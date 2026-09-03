#include "Core/Assets/MeshLoader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr std::size_t VertexHeaderSize =
        68;

    constexpr std::size_t IndexHeaderSize =
        72;

    constexpr std::size_t PrimitiveGroupSize =
        16;

    template<typename T>
    bool ReadValue(
        const std::span<const std::byte> data,
        const std::size_t offset,
        T& output) noexcept
    {
        if (offset > data.size())
        {
            return false;
        }

        if (sizeof(T) >
            data.size() - offset)
        {
            return false;
        }

        std::memcpy(
            &output,
            data.data() + offset,
            sizeof(T));

        return true;
    }

    bool ReadFixedString(
        const std::span<const std::byte> data,
        const std::size_t offset,
        const std::size_t maximumLength,
        std::string& output)
    {
        output.clear();

        if (offset >= data.size())
        {
            return false;
        }

        const std::size_t available =
            data.size() -
            offset;

        const std::size_t limit =
            std::min(
                maximumLength,
                available);

        for (std::size_t index = 0;
             index < limit;
             ++index)
        {
            const auto value =
                std::to_integer<unsigned char>(
                    data[offset + index]);

            if (value == 0)
            {
                return
                    !output.empty();
            }

            output.push_back(
                static_cast<char>(
                    value));
        }

        return false;
    }

    core::math::Vector3 Normalize(
        const core::math::Vector3 value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;

        if (lengthSquared <=
            0.0000001f)
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
            Normalize(value);

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

    const core::assets::PrimitivesSection*
    ResolveStreamSection(
        const core::assets::PrimitivesContainer& primitives,
        const std::string_view vertexSection,
        const std::string_view streamName)
    {
        //
        // Сначала точное имя.
        //
        if (const auto* section =
                primitives.FindSection(
                    streamName))
        {
            return section;
        }

        //
        // Для vertices вида:
        //
        // object.vertices
        //
        // дополнительные streams называются:
        //
        // object.uv2
        // object.colour
        //
        const std::size_t dot =
            vertexSection.find_last_of('.');

        if (dot ==
            std::string_view::npos)
        {
            return nullptr;
        }

        std::string resolved;

        resolved.reserve(
            dot +
            1 +
            streamName.size());

        resolved.append(
            vertexSection.substr(
                0,
                dot + 1));

        resolved.append(
            streamName);

        return
            primitives.FindSection(
                resolved);
    }

    bool ParseVertices(
        const core::assets::PrimitivesContainer& primitives,
        const core::assets::VisualGeometry& geometry,
        core::assets::MeshData& output,
        std::string& error)
    {
        const std::span<const std::byte> data =
            primitives.SectionData(
                geometry.vertexSection);

        if (data.empty())
        {
            error =
                "Vertex section was not found: " +
                geometry.vertexSection;

            return false;
        }

        if (data.size() <
            VertexHeaderSize)
        {
            error =
                "Vertex section is too small.";

            return false;
        }

        std::string format;

        if (!ReadFixedString(
                data,
                0,
                64,
                format))
        {
            error =
                "Unable to read vertex format.";

            return false;
        }

        std::uint32_t vertexCount = 0;

        if (!ReadValue(
                data,
                64,
                vertexCount))
        {
            error =
                "Unable to read vertex count.";

            return false;
        }

        if (vertexCount == 0)
        {
            error =
                "Vertex section contains zero vertices.";

            return false;
        }

        std::size_t vertexStride = 0;

        if (format == "xyznuvtb")
        {
            vertexStride = 32;
        }
        else if (format == "xyznuv")
        {
            vertexStride = 32;
        }
        else if (format == "xyznuv2tb")
        {
            vertexStride = 40;
        }
        else if (format == "xyznuv2")
        {
            vertexStride = 40;
        }
        else
        {
            error =
                "Unsupported vertex format: " +
                format;

            return false;
        }

        if (vertexCount >
            (
                std::numeric_limits<std::size_t>::max() -
                VertexHeaderSize
            ) /
            vertexStride)
        {
            error =
                "Vertex count is too large.";

            return false;
        }

        const std::size_t expectedSize =
            VertexHeaderSize +
            static_cast<std::size_t>(
                vertexCount) *
                vertexStride;

        if (data.size() !=
            expectedSize)
        {
            error =
                "Vertex section size does not match vertex count.";

            return false;
        }

        output.vertexFormat =
            format;

        output.vertices.clear();

        output.vertices.resize(
            vertexCount);

        for (std::size_t index = 0;
             index < output.vertices.size();
             ++index)
        {
            const std::size_t offset =
                VertexHeaderSize +
                index *
                    vertexStride;

            core::assets::MeshVertex& vertex =
                output.vertices[index];

            if (!ReadValue(
                    data,
                    offset + 0,
                    vertex.position.x) ||
                !ReadValue(
                    data,
                    offset + 4,
                    vertex.position.y) ||
                !ReadValue(
                    data,
                    offset + 8,
                    vertex.position.z))
            {
                error =
                    "Vertex position data is truncated.";

                return false;
            }

            if (format == "xyznuvtb")
            {
                if (!ReadValue(
                        data,
                        offset + 12,
                        vertex.packedNormal) ||
                    !ReadValue(
                        data,
                        offset + 16,
                        vertex.u) ||
                    !ReadValue(
                        data,
                        offset + 20,
                        vertex.v) ||
                    !ReadValue(
                        data,
                        offset + 24,
                        vertex.packedTangent) ||
                    !ReadValue(
                        data,
                        offset + 28,
                        vertex.packedBinormal))
                {
                    error =
                        "xyznuvtb vertex is truncated.";

                    return false;
                }

                continue;
            }

            if (format == "xyznuv")
            {
                core::math::Vector3 normal;

                if (!ReadValue(
                        data,
                        offset + 12,
                        normal.x) ||
                    !ReadValue(
                        data,
                        offset + 16,
                        normal.y) ||
                    !ReadValue(
                        data,
                        offset + 20,
                        normal.z) ||
                    !ReadValue(
                        data,
                        offset + 24,
                        vertex.u) ||
                    !ReadValue(
                        data,
                        offset + 28,
                        vertex.v))
                {
                    error =
                        "xyznuv vertex is truncated.";

                    return false;
                }

                vertex.packedNormal =
                    PackNormal(
                        normal);

                continue;
            }

            if (format == "xyznuv2tb")
            {
                if (!ReadValue(
                        data,
                        offset + 12,
                        vertex.packedNormal) ||
                    !ReadValue(
                        data,
                        offset + 16,
                        vertex.u) ||
                    !ReadValue(
                        data,
                        offset + 20,
                        vertex.v) ||
                    !ReadValue(
                        data,
                        offset + 24,
                        vertex.u2) ||
                    !ReadValue(
                        data,
                        offset + 28,
                        vertex.v2) ||
                    !ReadValue(
                        data,
                        offset + 32,
                        vertex.packedTangent) ||
                    !ReadValue(
                        data,
                        offset + 36,
                        vertex.packedBinormal))
                {
                    error =
                        "xyznuv2tb vertex is truncated.";

                    return false;
                }

                continue;
            }

            if (format == "xyznuv2")
            {
                core::math::Vector3 normal;

                if (!ReadValue(
                        data,
                        offset + 12,
                        normal.x) ||
                    !ReadValue(
                        data,
                        offset + 16,
                        normal.y) ||
                    !ReadValue(
                        data,
                        offset + 20,
                        normal.z) ||
                    !ReadValue(
                        data,
                        offset + 24,
                        vertex.u) ||
                    !ReadValue(
                        data,
                        offset + 28,
                        vertex.v) ||
                    !ReadValue(
                        data,
                        offset + 32,
                        vertex.u2) ||
                    !ReadValue(
                        data,
                        offset + 36,
                        vertex.v2))
                {
                    error =
                        "xyznuv2 vertex is truncated.";

                    return false;
                }

                vertex.packedNormal =
                    PackNormal(
                        normal);

                continue;
            }
        }

        return true;
    }

    bool ParseStreams(
        const core::assets::PrimitivesContainer& primitives,
        const core::assets::VisualGeometry& geometry,
        core::assets::MeshData& output,
        std::string& error)
    {
        std::unordered_set<std::string>
            processed;

        for (const std::string& streamName :
             geometry.streams)
        {
            if (!processed.insert(
                    streamName).second)
            {
                continue;
            }

            const core::assets::PrimitivesSection* section =
                ResolveStreamSection(
                    primitives,
                    geometry.vertexSection,
                    streamName);

            if (section == nullptr)
            {
                error =
                    "Vertex stream was not found: " +
                    streamName;

                return false;
            }

            const std::span<const std::byte> data =
                primitives.SectionData(
                    *section);

            if (data.empty())
            {
                error =
                    "Vertex stream is empty: " +
                    streamName;

                return false;
            }

            if (streamName == "colour")
            {
                const std::size_t expectedSize =
                    output.vertices.size() *
                    sizeof(std::uint32_t);

                if (data.size() !=
                    expectedSize)
                {
                    error =
                        "Colour stream size does not match vertex count.";

                    return false;
                }

                for (std::size_t index = 0;
                     index < output.vertices.size();
                     ++index)
                {
                    if (!ReadValue(
                            data,
                            index *
                                sizeof(std::uint32_t),
                            output.vertices[index].colour))
                    {
                        error =
                            "Colour stream is truncated.";

                        return false;
                    }
                }

                continue;
            }

            if (streamName == "uv2")
            {
                constexpr std::size_t UvStride =
                    sizeof(float) * 2;

                const std::size_t expectedSize =
                    output.vertices.size() *
                    UvStride;

                if (data.size() !=
                    expectedSize)
                {
                    error =
                        "UV2 stream size does not match vertex count.";

                    return false;
                }

                for (std::size_t index = 0;
                     index < output.vertices.size();
                     ++index)
                {
                    const std::size_t offset =
                        index *
                        UvStride;

                    if (!ReadValue(
                            data,
                            offset + 0,
                            output.vertices[index].u2) ||
                        !ReadValue(
                            data,
                            offset + 4,
                            output.vertices[index].v2))
                    {
                        error =
                            "UV2 stream is truncated.";

                        return false;
                    }
                }

                continue;
            }

            error =
                "Unsupported vertex stream: " +
                streamName;

            return false;
        }

        return true;
    }

    bool ParseIndices(
        const core::assets::PrimitivesContainer& primitives,
        const core::assets::VisualGeometry& geometry,
        core::assets::MeshData& output,
        std::string& error)
    {
        const std::span<const std::byte> data =
            primitives.SectionData(
                geometry.primitiveSection);

        if (data.empty())
        {
            error =
                "Index section was not found: " +
                geometry.primitiveSection;

            return false;
        }

        if (data.size() <
            IndexHeaderSize)
        {
            error =
                "Index section is too small.";

            return false;
        }

        std::string format;

        if (!ReadFixedString(
                data,
                0,
                64,
                format))
        {
            error =
                "Unable to read index format.";

            return false;
        }

        if (format != "list")
        {
            error =
                "Unsupported index format: " +
                format;

            return false;
        }

        std::uint32_t indexCount = 0;
        std::uint32_t primitiveGroupCount = 0;

        if (!ReadValue(
                data,
                64,
                indexCount) ||
            !ReadValue(
                data,
                68,
                primitiveGroupCount))
        {
            error =
                "Unable to read index section header.";

            return false;
        }

        if (indexCount == 0)
        {
            error =
                "Index section contains zero indices.";

            return false;
        }

        const std::size_t indexDataSize =
            static_cast<std::size_t>(
                indexCount) *
            sizeof(std::uint16_t);

        const std::size_t groupDataSize =
            static_cast<std::size_t>(
                primitiveGroupCount) *
            PrimitiveGroupSize;

        const std::size_t afterIndices =
            IndexHeaderSize +
            indexDataSize;

        const std::size_t expectedSize =
            afterIndices +
            groupDataSize;

        if (data.size() !=
            expectedSize)
        {
            error =
                "Index section size does not match header.";

            return false;
        }

        if (geometry.primitiveGroups.size() !=
            static_cast<std::size_t>(
                primitiveGroupCount))
        {
            error =
                "Visual primitive group count does not match index data.";

            return false;
        }

        output.indices.clear();

        output.indices.resize(
            indexCount);

        for (std::size_t index = 0;
             index < output.indices.size();
             ++index)
        {
            std::uint16_t value = 0;

            if (!ReadValue(
                    data,
                    IndexHeaderSize +
                        index *
                            sizeof(std::uint16_t),
                    value))
            {
                error =
                    "Index data is truncated.";

                return false;
            }

            if (value >=
                output.vertices.size())
            {
                error =
                    "Index references vertex outside vertex buffer.";

                return false;
            }

            output.indices[index] =
                value;
        }

        output.primitiveGroups.clear();

        output.primitiveGroups.resize(
            primitiveGroupCount);

        const std::size_t groupOffset =
            IndexHeaderSize +
            indexDataSize;

        for (std::size_t index = 0;
             index <
                output.primitiveGroups.size();
             ++index)
        {
            const std::size_t offset =
                groupOffset +
                index *
                    PrimitiveGroupSize;

            core::assets::MeshPrimitiveGroup& group =
                output.primitiveGroups[index];

            if (!ReadValue(
                    data,
                    offset + 0,
                    group.startIndex) ||
                !ReadValue(
                    data,
                    offset + 4,
                    group.primitiveCount) ||
                !ReadValue(
                    data,
                    offset + 8,
                    group.startVertex) ||
                !ReadValue(
                    data,
                    offset + 12,
                    group.vertexCount))
            {
                error =
                    "Primitive group data is truncated.";

                return false;
            }

            const std::uint64_t usedIndices =
                static_cast<std::uint64_t>(
                    group.primitiveCount) *
                3ull;

            const std::uint64_t endIndex =
                static_cast<std::uint64_t>(
                    group.startIndex) +
                usedIndices;

            if (endIndex >
                output.indices.size())
            {
                error =
                    "Primitive group exceeds index buffer.";

                return false;
            }

            const std::uint64_t endVertex =
                static_cast<std::uint64_t>(
                    group.startVertex) +
                static_cast<std::uint64_t>(
                    group.vertexCount);

            if (endVertex >
                output.vertices.size())
            {
                error =
                    "Primitive group exceeds vertex buffer.";

                return false;
            }

            const std::int32_t visualGroup =
                geometry.primitiveGroups[index].index;

            if (visualGroup < 0 ||
                static_cast<std::uint32_t>(
                    visualGroup) >=
                    primitiveGroupCount)
            {
                error =
                    "Visual contains invalid primitive group index.";

                return false;
            }
        }

        return true;
    }
}

namespace core::assets
{
    bool MeshLoader::Load(
        const PrimitivesContainer& primitives,
        const VisualGeometry& geometry,
        MeshData& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        MeshData mesh;

        if (!ParseVertices(
                primitives,
                geometry,
                mesh,
                error))
        {
            return false;
        }

        if (!ParseStreams(
                primitives,
                geometry,
                mesh,
                error))
        {
            return false;
        }

        if (!ParseIndices(
                primitives,
                geometry,
                mesh,
                error))
        {
            return false;
        }

        output =
            std::move(mesh);

        return true;
    }
}