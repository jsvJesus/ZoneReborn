#include "Core/Assets/MeshLoader.h"

#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr std::size_t VertexHeaderSize = 68;
    constexpr std::size_t VertexStride = 32;

    constexpr std::size_t IndexHeaderSize = 72;
    constexpr std::size_t PrimitiveGroupSize = 16;

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
            data.size() - offset;

        const std::size_t limit =
            maximumLength < available
                ? maximumLength
                : available;

        for (std::size_t index = 0;
             index < limit;
             ++index)
        {
            const unsigned char value =
                std::to_integer<unsigned char>(
                    data[offset + index]);

            if (value == 0)
            {
                return !output.empty();
            }

            output.push_back(
                static_cast<char>(value));
        }

        return false;
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

        if (data.size() < VertexHeaderSize)
        {
            error =
                "Vertex section is too small.";

            return false;
        }

        std::string format;

        if (!ReadFixedString(
                data,
                0,
                12,
                format))
        {
            error =
                "Unable to read vertex format.";

            return false;
        }

        if (format != "xyznuvtb")
        {
            error =
                "Unsupported vertex format: " +
                format;

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

        if (vertexCount >
            (std::numeric_limits<std::size_t>::max() -
             VertexHeaderSize) /
                VertexStride)
        {
            error =
                "Vertex count is too large.";

            return false;
        }

        const std::size_t expectedSize =
            VertexHeaderSize +
            static_cast<std::size_t>(
                vertexCount) *
                VertexStride;

        if (data.size() != expectedSize)
        {
            error =
                "Vertex section size does not match vertex count.";

            return false;
        }

        output.vertexFormat =
            std::move(format);

        output.vertices.clear();

        output.vertices.resize(
            vertexCount);

        for (std::size_t index = 0;
             index < output.vertices.size();
             ++index)
        {
            const std::size_t offset =
                VertexHeaderSize +
                index * VertexStride;

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
                    vertex.position.z) ||
                !ReadValue(
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
                    "Vertex data is truncated.";

                return false;
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
        std::unordered_set<std::string> processed;

        for (const std::string& streamName :
             geometry.streams)
        {
            if (!processed.insert(
                    streamName).second)
            {
                continue;
            }

            if (streamName != "colour")
            {
                error =
                    "Unsupported vertex stream: " +
                    streamName;

                return false;
            }

            const std::span<const std::byte> data =
                primitives.SectionData(
                    streamName);

            if (data.empty())
            {
                error =
                    "Vertex stream was not found: " +
                    streamName;

                return false;
            }

            const std::size_t expectedSize =
                output.vertices.size() *
                sizeof(std::uint32_t);

            if (data.size() != expectedSize)
            {
                error =
                    "Colour stream size does not match vertex count.";

                return false;
            }

            for (std::size_t index = 0;
                 index < output.vertices.size();
                 ++index)
            {
                std::uint32_t colour = 0;

                if (!ReadValue(
                        data,
                        index *
                            sizeof(std::uint32_t),
                        colour))
                {
                    error =
                        "Colour stream is truncated.";

                    return false;
                }

                output.vertices[index].colour =
                    colour;
            }
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

        if (data.size() < IndexHeaderSize)
        {
            error =
                "Index section is too small.";

            return false;
        }

        std::string format;

        if (!ReadFixedString(
                data,
                0,
                8,
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

        if (indexDataSize >
            std::numeric_limits<std::size_t>::max() -
                IndexHeaderSize)
        {
            error =
                "Index count is too large.";

            return false;
        }

        const std::size_t afterIndices =
            IndexHeaderSize +
            indexDataSize;

        if (groupDataSize >
            std::numeric_limits<std::size_t>::max() -
                afterIndices)
        {
            error =
                "Primitive group count is too large.";

            return false;
        }

        const std::size_t expectedSize =
            afterIndices +
            groupDataSize;

        if (data.size() != expectedSize)
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
             index < output.primitiveGroups.size();
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