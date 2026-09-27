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

    // Newer character meshes may keep one morph target directly after the
    // ordinary skinned vertex buffer. The current renderer does not animate
    // morph targets yet, but the base vertices remain fully usable.
    constexpr std::size_t MorphTargetHeaderSize =
        80;

    constexpr std::size_t MorphTargetVertexStride =
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

    bool ReadSkinningData(
        const std::span<const std::byte> data,
        const std::size_t offset,
        core::assets::MeshVertex& vertex) noexcept
    {
        std::uint8_t index0 = 0;
        std::uint8_t index1 = 0;
        std::uint8_t index2 = 0;

        std::uint8_t weight0 = 0;
        std::uint8_t weight1 = 0;

        if (!ReadValue(
                data,
                offset + 24,
                index0) ||
            !ReadValue(
                data,
                offset + 25,
                index1) ||
            !ReadValue(
                data,
                offset + 26,
                index2) ||
            !ReadValue(
                data,
                offset + 27,
                weight0) ||
            !ReadValue(
                data,
                offset + 28,
                weight1))
        {
            return false;
        }

        //
        // BigWorld хранит не номер bone,
        // а offset в float4 palette:
        //
        // 0, 3, 6, 9...
        //
        vertex.boneIndices[0] =
            static_cast<std::uint16_t>(
                index0 / 3u);

        vertex.boneIndices[1] =
            static_cast<std::uint16_t>(
                index1 / 3u);

        vertex.boneIndices[2] =
            static_cast<std::uint16_t>(
                index2 / 3u);

        vertex.boneWeights[0] =
            static_cast<float>(
                weight0) /
            255.0f;

        vertex.boneWeights[1] =
            static_cast<float>(
                weight1) /
            255.0f;

        vertex.boneWeights[2] =
            1.0f -
            vertex.boneWeights[0] -
            vertex.boneWeights[1];

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

        std::string_view StreamLeaf(
        const std::string_view streamName) noexcept
    {
        const std::size_t dot =
            streamName.find_last_of('.');

        if (dot !=
                std::string_view::npos &&
            dot + 1 <
                streamName.size())
        {
            return
                streamName.substr(
                    dot + 1);
        }

        return
            streamName;
    }

    const core::assets::PrimitivesSection*
    ResolveStreamSection(
        const core::assets::PrimitivesContainer& primitives,
        const std::string_view vertexSection,
        const std::string_view streamName)
    {
        if (streamName.empty())
        {
            return nullptr;
        }

        if (const auto* section =
                primitives.FindSection(
                    streamName))
        {
            return section;
        }

        const std::string_view streamLeaf =
            StreamLeaf(
                streamName);

        if (streamLeaf.empty())
        {
            return nullptr;
        }

        const std::size_t vertexDot =
            vertexSection.find_last_of('.');

        std::string stem;

        if (vertexDot !=
            std::string_view::npos)
        {
            stem.assign(
                vertexSection.substr(
                    0,
                    vertexDot));

            {
                std::string candidate =
                    stem;

                candidate.push_back('.');
                candidate.append(
                    streamLeaf);

                if (const auto* section =
                        primitives.FindSection(
                            candidate))
                {
                    return section;
                }
            }

            {
                std::string candidate =
                    stem;

                candidate.push_back('_');
                candidate.append(
                    streamLeaf);

                if (const auto* section =
                        primitives.FindSection(
                            candidate))
                {
                    return section;
                }
            }
        }

        {
            std::string candidate(
                vertexSection);

            candidate.push_back('.');
            candidate.append(
                streamLeaf);

            if (const auto* section =
                    primitives.FindSection(
                        candidate))
            {
                return section;
            }
        }

        {
            std::string candidate(
                vertexSection);

            candidate.push_back('_');
            candidate.append(
                streamLeaf);

            if (const auto* section =
                    primitives.FindSection(
                        candidate))
            {
                return section;
            }
        }

        const std::string dotSuffix =
            "." +
            std::string(
                streamLeaf);

        const std::string underscoreSuffix =
            "_" +
            std::string(
                streamLeaf);

        for (const core::assets::PrimitivesSection& section :
             primitives.sections)
        {
            if (!stem.empty())
            {
                if (!section.name.starts_with(
                        stem))
                {
                    continue;
                }

                if (section.name.size() <=
                    stem.size())
                {
                    continue;
                }

                const char boundary =
                    section.name[
                        stem.size()];

                if (boundary != '.' &&
                    boundary != '_')
                {
                    continue;
                }
            }

            if (section.name.ends_with(
                    dotSuffix) ||
                section.name.ends_with(
                    underscoreSuffix))
            {
                return
                    &section;
            }
        }

        return nullptr;
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
        else if (format == "xyznuviiiwwtb")
        {
            vertexStride = 37;
        }
        else if (format == "xyznuviiiww")
        {
            vertexStride = 29;
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

        bool validSectionSize =
            data.size() ==
                expectedSize;

        if (!validSectionSize &&
            geometry.vertexSection.ends_with(
                ".mvertices") &&
            expectedSize <=
                std::numeric_limits<std::size_t>::max() -
                    MorphTargetHeaderSize &&
            vertexCount <=
                (
                    std::numeric_limits<std::size_t>::max() -
                    expectedSize -
                    MorphTargetHeaderSize
                ) /
                MorphTargetVertexStride)
        {
            const std::size_t expectedMorphSize =
                expectedSize +
                MorphTargetHeaderSize +
                static_cast<std::size_t>(
                    vertexCount) *
                    MorphTargetVertexStride;

            validSectionSize =
                data.size() ==
                    expectedMorphSize;
        }

        if (!validSectionSize)
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

            if (format == "xyznuviiiwwtb")
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
                    !ReadSkinningData(
                        data,
                        offset,
                        vertex) ||
                    !ReadValue(
                        data,
                        offset + 29,
                        vertex.packedTangent) ||
                    !ReadValue(
                        data,
                        offset + 33,
                        vertex.packedBinormal))
                {
                    error =
                        "xyznuviiiwwtb vertex is truncated.";

                    return false;
                }

                output.skinned =
                    true;

                continue;
            }

            if (format == "xyznuviiiww")
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
                    !ReadSkinningData(
                        data,
                        offset,
                        vertex))
                {
                    error =
                        "xyznuviiiww vertex is truncated.";

                    return false;
                }

                output.skinned =
                    true;

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

            const std::string_view streamLeaf =
                StreamLeaf(
                    streamName);

            const bool isColourStream =
                streamLeaf == "colour" ||
                streamLeaf == "color";

            const bool isUv2Stream =
                streamLeaf == "uv2";

            if (!isColourStream &&
                !isUv2Stream)
            {
                error =
                    "Unsupported vertex stream: " +
                    streamName;

                return false;
            }

            const core::assets::PrimitivesSection* section =
                ResolveStreamSection(
                    primitives,
                    geometry.vertexSection,
                    streamName);

            if (section == nullptr)
            {
                //
                // Эти streams являются дополнительными.
                //
                // Для colour MeshVertex уже имеет
                // нейтральный белый цвет.
                //
                // Для uv2 MeshVertex уже имеет 0,0.
                //
                // Некоторые ресурсы содержат ссылку
                // на stream в visual, хотя самого
                // stream blob в primitives нет.
                //
                continue;
            }

            const std::span<const std::byte> data =
                primitives.SectionData(
                    *section);

            if (data.empty())
            {
                error =
                    "Vertex stream is empty: " +
                    section->name;

                return false;
            }

            if (isColourStream)
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
                     index <
                        output.vertices.size();
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

            constexpr std::size_t UvStride =
                sizeof(float) *
                2;

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
                 index <
                    output.vertices.size();
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

        core::assets::MeshIndexFormat indexFormat;
        std::size_t indexStride = 0;

        if (format == "list")
        {
            indexFormat =
                core::assets::
                    MeshIndexFormat::UInt16;

            indexStride =
                sizeof(std::uint16_t);
        }
        else if (format == "list32")
        {
            indexFormat =
                core::assets::
                    MeshIndexFormat::UInt32;

            indexStride =
                sizeof(std::uint32_t);
        }
        else
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

        if (static_cast<std::size_t>(
                indexCount) >
            (
                std::numeric_limits<std::size_t>::max() -
                IndexHeaderSize
            ) /
            indexStride)
        {
            error =
                "Index count is too large.";

            return false;
        }

        const std::size_t indexDataSize =
            static_cast<std::size_t>(
                indexCount) *
            indexStride;

        const std::size_t afterIndices =
            IndexHeaderSize +
            indexDataSize;

        if (static_cast<std::size_t>(
                primitiveGroupCount) >
            (
                std::numeric_limits<std::size_t>::max() -
                afterIndices
            ) /
            PrimitiveGroupSize)
        {
            error =
                "Primitive group count is too large.";

            return false;
        }

        const std::size_t groupDataSize =
            static_cast<std::size_t>(
                primitiveGroupCount) *
            PrimitiveGroupSize;

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

        output.indexFormat =
            indexFormat;

        output.indices.clear();
        output.indices32.clear();

        if (indexFormat ==
            core::assets::
                MeshIndexFormat::UInt32)
        {
            output.indices32.resize(
                indexCount);
        }
        else
        {
            output.indices.resize(
                indexCount);
        }

        for (std::size_t index = 0;
             index <
                static_cast<std::size_t>(
                    indexCount);
             ++index)
        {
            std::uint32_t value = 0;

            const std::size_t offset =
                IndexHeaderSize +
                index *
                    indexStride;

            if (indexFormat ==
                core::assets::
                    MeshIndexFormat::UInt32)
            {
                if (!ReadValue(
                        data,
                        offset,
                        value))
                {
                    error =
                        "32-bit index data is truncated.";

                    return false;
                }
            }
            else
            {
                std::uint16_t value16 = 0;

                if (!ReadValue(
                        data,
                        offset,
                        value16))
                {
                    error =
                        "16-bit index data is truncated.";

                    return false;
                }

                value =
                    value16;
            }

            if (static_cast<std::size_t>(
                    value) >=
                output.vertices.size())
            {
                error =
                    "Index references vertex outside vertex buffer.";

                return false;
            }

            if (indexFormat ==
                core::assets::
                    MeshIndexFormat::UInt32)
            {
                output.indices32[index] =
                    value;
            }
            else
            {
                output.indices[index] =
                    static_cast<std::uint16_t>(
                        value);
            }
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
                output.primitiveGroups[
                    index];

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
                output.IndexCount())
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
        }

        //
        // Binary primitive groups и visual material groups
        // не обязаны иметь одинаковое количество.
        //
        // Binary section описывает физические диапазоны
        // индексов, visual выбирает используемые группы
        // и назначает им материалы.
        //
        if (!geometry.primitiveGroups.empty())
        {
            for (core::assets::MeshPrimitiveGroup& group :
                 output.primitiveGroups)
            {
                group.renderEnabled =
                    false;
            }
        }

        for (const core::assets::VisualPrimitiveGroup& visualGroup :
             geometry.primitiveGroups)
        {
            if (visualGroup.index < 0)
            {
                error =
                    "Visual contains negative primitive group index.";

                return false;
            }

            const std::size_t groupIndex =
                static_cast<std::size_t>(
                    visualGroup.index);

            if (groupIndex >=
                output.primitiveGroups.size())
            {
                error =
                    "Visual contains invalid primitive group index.";

                return false;
            }

            output.primitiveGroups[
                groupIndex].renderEnabled =
                    true;
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
