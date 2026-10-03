#include "Core/World/XRay/XRayLevelLoader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace
{
    constexpr std::uint32_t CompressionFlag =
        0x80000000u;

    constexpr std::uint16_t SupportedLevelVersion =
        14u;

    constexpr std::uint8_t SupportedOgfVersion =
        4u;

    constexpr std::uint32_t LevelHeaderChunk = 1u;
    constexpr std::uint32_t LevelShadersChunk = 2u;
    constexpr std::uint32_t LevelVisualsChunk = 3u;
    constexpr std::uint32_t LevelVertexBuffersChunk = 9u;
    constexpr std::uint32_t LevelIndexBuffersChunk = 10u;

    constexpr std::uint32_t OgfHeaderChunk = 1u;
    constexpr std::uint32_t OgfChildrenLinksChunk = 10u;
    constexpr std::uint32_t OgfGeometryContainerChunk = 21u;

    constexpr std::uint8_t DeclarationTypeUnused =
        17u;

    struct Range final
    {
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
    };

    struct Chunk final
    {
        Range data;

        std::uint32_t id = 0;
        bool compressed = false;
    };

    bool CheckedAdd(
        const std::uint64_t left,
        const std::uint64_t right,
        std::uint64_t& result) noexcept
    {
        if (right >
            std::numeric_limits<std::uint64_t>::max() -
                left)
        {
            return false;
        }

        result =
            left +
            right;

        return true;
    }

    bool CheckedMultiply(
        const std::uint64_t left,
        const std::uint64_t right,
        std::uint64_t& result) noexcept
    {
        if (left != 0 &&
            right >
                std::numeric_limits<std::uint64_t>::max() /
                    left)
        {
            return false;
        }

        result =
            left *
            right;

        return true;
    }

    class BinaryFile final
    {
    public:
        bool Open(
            const std::filesystem::path& path,
            std::string& error)
        {
            path_ =
                path;

            stream_.open(
                path,
                std::ios::binary);

            if (!stream_)
            {
                error =
                    "Unable to open X-Ray level file: " +
                    path.string();

                return false;
            }

            stream_.seekg(
                0,
                std::ios::end);

            const std::streampos end =
                stream_.tellg();

            if (end < 0)
            {
                error =
                    "Unable to determine X-Ray level file size: " +
                    path.string();

                return false;
            }

            size_ =
                static_cast<std::uint64_t>(
                    end);

            stream_.clear();

            return true;
        }

        [[nodiscard]]
        std::uint64_t Size() const noexcept
        {
            return size_;
        }

        bool Read(
            const std::uint64_t offset,
            void* destination,
            const std::size_t size,
            std::string& error)
        {
            std::uint64_t end = 0;

            if (!CheckedAdd(
                    offset,
                    size,
                    end) ||
                end >
                    size_)
            {
                error =
                    "Read exceeds X-Ray level file bounds: " +
                    path_.string();

                return false;
            }

            if (offset >
                static_cast<std::uint64_t>(
                    std::numeric_limits<std::streamoff>::max()))
            {
                error =
                    "X-Ray level file offset is too large: " +
                    path_.string();

                return false;
            }

            stream_.clear();
            stream_.seekg(
                static_cast<std::streamoff>(
                    offset),
                std::ios::beg);

            if (!stream_)
            {
                error =
                    "Unable to seek in X-Ray level file: " +
                    path_.string();

                return false;
            }

            if (size != 0)
            {
                stream_.read(
                    static_cast<char*>(
                        destination),
                    static_cast<std::streamsize>(
                        size));

                if (!stream_)
                {
                    error =
                        "Unable to read X-Ray level file: " +
                        path_.string();

                    return false;
                }
            }

            return true;
        }

        template<typename Value>
        bool ReadValue(
            const std::uint64_t offset,
            Value& value,
            std::string& error)
        {
            return
                Read(
                    offset,
                    &value,
                    sizeof(value),
                    error);
        }

    private:
        std::filesystem::path path_;
        std::ifstream stream_;
        std::uint64_t size_ = 0;
    };

    bool RangeEnd(
        const Range& range,
        std::uint64_t& end,
        std::string& error,
        const std::string& context)
    {
        if (!CheckedAdd(
                range.offset,
                range.size,
                end))
        {
            error =
                context +
                " range overflows.";

            return false;
        }

        return true;
    }

    bool FindChunk(
        BinaryFile& file,
        const Range& parent,
        const std::uint32_t requestedId,
        Chunk& output,
        bool& found,
        std::string& error,
        const std::string& context)
    {
        found =
            false;

        std::uint64_t parentEnd = 0;

        if (!RangeEnd(
                parent,
                parentEnd,
                error,
                context))
        {
            return false;
        }

        if (parentEnd >
            file.Size())
        {
            error =
                context +
                " range exceeds the file.";

            return false;
        }

        std::uint64_t cursor =
            parent.offset;

        while (cursor <
               parentEnd)
        {
            if (parentEnd -
                    cursor <
                8u)
            {
                error =
                    context +
                    " contains a truncated chunk header.";

                return false;
            }

            std::array<std::uint32_t, 2>
                header{};

            if (!file.Read(
                    cursor,
                    header.data(),
                    sizeof(header),
                    error))
            {
                return false;
            }

            const std::uint32_t rawId =
                header[0];

            const std::uint64_t dataOffset =
                cursor +
                sizeof(header);

            std::uint64_t chunkEnd = 0;

            if (!CheckedAdd(
                    dataOffset,
                    header[1],
                    chunkEnd) ||
                chunkEnd >
                    parentEnd)
            {
                error =
                    context +
                    " contains an out-of-bounds chunk.";

                return false;
            }

            const std::uint32_t id =
                rawId &
                ~CompressionFlag;

            if (id ==
                requestedId)
            {
                output.id =
                    id;

                output.compressed =
                    (rawId & CompressionFlag) != 0;

                output.data.offset =
                    dataOffset;

                output.data.size =
                    header[1];

                found =
                    true;

                return true;
            }

            cursor =
                chunkEnd;
        }

        return true;
    }

    bool RequireChunk(
        BinaryFile& file,
        const Range& parent,
        const std::uint32_t id,
        Chunk& output,
        std::string& error,
        const std::string& context)
    {
        bool found =
            false;

        if (!FindChunk(
                file,
                parent,
                id,
                output,
                found,
                error,
                context))
        {
            return false;
        }

        if (!found)
        {
            error =
                context +
                " is missing required chunk " +
                std::to_string(id) +
                ".";

            return false;
        }

        if (output.compressed)
        {
            error =
                context +
                " uses unsupported compressed chunk " +
                std::to_string(id) +
                ".";

            return false;
        }

        return true;
    }

    std::uint32_t DeclarationTypeSize(
        const std::uint8_t type) noexcept
    {
        switch (type)
        {
        case 0: return 4u;
        case 1: return 8u;
        case 2: return 12u;
        case 3: return 16u;
        case 4: return 4u;
        case 5: return 4u;
        case 6: return 4u;
        case 7: return 8u;
        case 8: return 4u;
        case 9: return 4u;
        case 10: return 8u;
        case 11: return 4u;
        case 12: return 8u;
        case 13: return 4u;
        case 14: return 4u;
        case 15: return 4u;
        case 16: return 8u;
        default: return 0u;
        }
    }

    bool ParseShaders(
        BinaryFile& file,
        const Chunk& chunk,
        core::world::xray::LevelData& output,
        std::string& error)
    {
        if (chunk.data.size <
                sizeof(std::uint32_t) ||
            chunk.data.size >
                static_cast<std::uint64_t>(
                    std::numeric_limits<std::size_t>::max()))
        {
            error =
                "X-Ray shader table has an invalid size.";

            return false;
        }

        std::vector<std::byte> data(
            static_cast<std::size_t>(
                chunk.data.size));

        if (!file.Read(
                chunk.data.offset,
                data.data(),
                data.size(),
                error))
        {
            return false;
        }

        std::uint32_t shaderCount = 0;

        std::memcpy(
            &shaderCount,
            data.data(),
            sizeof(shaderCount));

        if (shaderCount >
            data.size() -
                sizeof(shaderCount))
        {
            error =
                "X-Ray shader table count exceeds its chunk.";

            return false;
        }

        output.shaderCount =
            shaderCount;

        output.shaders.clear();
        output.shaders.reserve(
            shaderCount);

        std::size_t cursor =
            sizeof(shaderCount);

        for (std::uint32_t shaderIndex = 0;
             shaderIndex <
                shaderCount;
             ++shaderIndex)
        {
            const void* terminator =
                std::memchr(
                    data.data() +
                        cursor,
                    0,
                    data.size() -
                        cursor);

            if (terminator ==
                nullptr)
            {
                error =
                    "X-Ray shader table entry " +
                    std::to_string(
                        shaderIndex) +
                    " is not null terminated.";

                return false;
            }

            const auto* entryEnd =
                static_cast<const std::byte*>(
                    terminator);

            const char* entryBegin =
                reinterpret_cast<const char*>(
                    data.data() +
                    cursor);

            const std::size_t entrySize =
                static_cast<std::size_t>(
                    entryEnd -
                    (data.data() +
                     cursor));

            const std::string entry(
                entryBegin,
                entrySize);

            core::world::xray::ShaderReference
                shader;

            const std::size_t separator =
                entry.find('/');

            if (separator ==
                std::string::npos)
            {
                shader.renderer =
                    entry;
            }
            else
            {
                shader.renderer =
                    entry.substr(
                        0u,
                        separator);

                const std::string textureList =
                    entry.substr(
                        separator +
                        1u);

                std::size_t textureStart =
                    0u;

                while (textureStart <=
                       textureList.size())
                {
                    const std::size_t textureEnd =
                        textureList.find(
                            ',',
                            textureStart);

                    const std::size_t length =
                        textureEnd ==
                            std::string::npos
                            ? textureList.size() -
                                textureStart
                            : textureEnd -
                                textureStart;

                    if (length !=
                        0u)
                    {
                        shader.textures.push_back(
                            textureList.substr(
                                textureStart,
                                length));
                    }

                    if (textureEnd ==
                        std::string::npos)
                    {
                        break;
                    }

                    textureStart =
                        textureEnd +
                        1u;
                }
            }

            output.shaders.push_back(
                std::move(
                    shader));

            cursor =
                static_cast<std::size_t>(
                    entryEnd -
                    data.data()) +
                1u;
        }

        return true;
    }

    bool ParseVertexBuffers(
        BinaryFile& file,
        const Chunk& chunk,
        core::world::xray::LevelData& output,
        std::string& error)
    {
        if (chunk.data.size <
            sizeof(std::uint32_t))
        {
            error =
                "X-Ray vertex-buffer chunk is truncated.";

            return false;
        }

        std::uint32_t bufferCount = 0;

        if (!file.ReadValue(
                chunk.data.offset,
                bufferCount,
                error))
        {
            return false;
        }

        output.vertexBuffers.clear();
        output.vertexBuffers.reserve(
            bufferCount);

        std::uint64_t chunkEnd = 0;

        if (!RangeEnd(
                chunk.data,
                chunkEnd,
                error,
                "X-Ray vertex-buffer chunk"))
        {
            return false;
        }

        std::uint64_t cursor =
            chunk.data.offset +
            sizeof(std::uint32_t);

        for (std::uint32_t bufferIndex = 0;
             bufferIndex <
                bufferCount;
             ++bufferIndex)
        {
            core::world::xray::VertexBuffer
                buffer;

            bool terminated =
                false;

            for (std::uint32_t elementIndex = 0;
                 elementIndex <
                    65u;
                 ++elementIndex)
            {
                if (chunkEnd -
                        cursor <
                    8u)
                {
                    error =
                        "X-Ray vertex declaration is truncated.";

                    return false;
                }

                std::array<std::byte, 8>
                    bytes{};

                if (!file.Read(
                        cursor,
                        bytes.data(),
                        bytes.size(),
                        error))
                {
                    return false;
                }

                core::world::xray::VertexElement
                    element;

                std::memcpy(
                    &element.stream,
                    bytes.data() + 0,
                    sizeof(element.stream));

                std::memcpy(
                    &element.offset,
                    bytes.data() + 2,
                    sizeof(element.offset));

                element.type =
                    std::to_integer<std::uint8_t>(
                        bytes[4]);

                element.method =
                    std::to_integer<std::uint8_t>(
                        bytes[5]);

                element.usage =
                    std::to_integer<std::uint8_t>(
                        bytes[6]);

                element.usageIndex =
                    std::to_integer<std::uint8_t>(
                        bytes[7]);

                cursor +=
                    bytes.size();

                if (element.stream ==
                        0x00FFu &&
                    element.type ==
                        DeclarationTypeUnused)
                {
                    terminated =
                        true;

                    break;
                }

                if (element.stream !=
                    0u)
                {
                    error =
                        "X-Ray vertex buffer uses multiple streams, which are not supported.";

                    return false;
                }

                const std::uint32_t elementSize =
                    DeclarationTypeSize(
                        element.type);

                if (elementSize ==
                    0u)
                {
                    error =
                        "X-Ray vertex declaration uses unsupported element type " +
                        std::to_string(element.type) +
                        ".";

                    return false;
                }

                buffer.stride =
                    std::max(
                        buffer.stride,
                        static_cast<std::uint32_t>(
                            element.offset) +
                            elementSize);

                buffer.elements.push_back(
                    element);
            }

            if (!terminated)
            {
                error =
                    "X-Ray vertex declaration has no terminator.";

                return false;
            }

            if (buffer.elements.empty() ||
                buffer.stride ==
                    0u)
            {
                error =
                    "X-Ray vertex declaration is empty.";

                return false;
            }

            if (chunkEnd -
                    cursor <
                sizeof(std::uint32_t))
            {
                error =
                    "X-Ray vertex buffer is missing its vertex count.";

                return false;
            }

            if (!file.ReadValue(
                    cursor,
                    buffer.vertexCount,
                    error))
            {
                return false;
            }

            cursor +=
                sizeof(std::uint32_t);

            std::uint64_t byteCount = 0;

            if (!CheckedMultiply(
                    buffer.vertexCount,
                    buffer.stride,
                    byteCount) ||
                byteCount >
                    chunkEnd -
                        cursor)
            {
                error =
                    "X-Ray vertex buffer exceeds its chunk.";

                return false;
            }

            buffer.dataOffset =
                cursor;

            cursor +=
                byteCount;

            output.vertexBuffers.push_back(
                std::move(
                    buffer));
        }

        return true;
    }

    bool ParseIndexBuffers(
        BinaryFile& file,
        const Chunk& chunk,
        core::world::xray::LevelData& output,
        std::string& error)
    {
        if (chunk.data.size <
            sizeof(std::uint32_t))
        {
            error =
                "X-Ray index-buffer chunk is truncated.";

            return false;
        }

        std::uint32_t bufferCount = 0;

        if (!file.ReadValue(
                chunk.data.offset,
                bufferCount,
                error))
        {
            return false;
        }

        output.indexBuffers.clear();
        output.indexBuffers.reserve(
            bufferCount);

        std::uint64_t chunkEnd = 0;

        if (!RangeEnd(
                chunk.data,
                chunkEnd,
                error,
                "X-Ray index-buffer chunk"))
        {
            return false;
        }

        std::uint64_t cursor =
            chunk.data.offset +
            sizeof(std::uint32_t);

        for (std::uint32_t bufferIndex = 0;
             bufferIndex <
                bufferCount;
             ++bufferIndex)
        {
            if (chunkEnd -
                    cursor <
                sizeof(std::uint32_t))
            {
                error =
                    "X-Ray index buffer is missing its index count.";

                return false;
            }

            core::world::xray::IndexBuffer
                buffer;

            if (!file.ReadValue(
                    cursor,
                    buffer.indexCount,
                    error))
            {
                return false;
            }

            cursor +=
                sizeof(std::uint32_t);

            std::uint64_t byteCount = 0;

            if (!CheckedMultiply(
                    buffer.indexCount,
                    sizeof(std::uint16_t),
                    byteCount) ||
                byteCount >
                    chunkEnd -
                        cursor)
            {
                error =
                    "X-Ray index buffer exceeds its chunk.";

                return false;
            }

            buffer.dataOffset =
                cursor;

            cursor +=
                byteCount;

            output.indexBuffers.push_back(
                buffer);
        }

        return true;
    }

    bool ParseVisuals(
        BinaryFile& file,
        const Chunk& chunk,
        core::world::xray::LevelData& output,
        std::string& error)
    {
        if (chunk.data.size >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()))
        {
            error =
                "X-Ray visuals chunk is too large for memory.";

            return false;
        }

        std::vector<std::byte> data(
            static_cast<std::size_t>(
                chunk.data.size));

        if (!file.Read(
                chunk.data.offset,
                data.data(),
                data.size(),
                error))
        {
            return false;
        }

        output.visuals.clear();

        std::uint64_t cursor =
            0u;

        const std::uint64_t chunkEnd =
            data.size();

        while (cursor <
               chunkEnd)
        {
            if (chunkEnd -
                    cursor <
                8u)
            {
                error =
                    "X-Ray visuals chunk contains a truncated visual header.";

                return false;
            }

            std::array<std::uint32_t, 2>
                header{};

            std::memcpy(
                header.data(),
                data.data() +
                    static_cast<std::size_t>(
                        cursor),
                sizeof(header));

            if ((header[0] &
                 CompressionFlag) !=
                0u)
            {
                error =
                    "X-Ray visual uses unsupported chunk compression.";

                return false;
            }

            const std::uint64_t dataOffset =
                cursor +
                sizeof(header);

            std::uint64_t visualEnd = 0;

            if (!CheckedAdd(
                    dataOffset,
                    header[1],
                    visualEnd) ||
                visualEnd >
                    chunkEnd)
            {
                error =
                    "X-Ray visual exceeds the visuals chunk.";

                return false;
            }

            core::world::xray::Visual
                visual;

            const std::string context =
                "X-Ray visual " +
                std::to_string(
                    output.visuals.size());

            bool headerFound =
                false;

            bool geometryFound =
                false;

            bool childrenFound =
                false;

            std::uint64_t visualCursor =
                dataOffset;

            while (visualCursor <
                   visualEnd)
            {
                if (visualEnd -
                        visualCursor <
                    8u)
                {
                    error =
                        context +
                        " contains a truncated chunk header.";

                    return false;
                }

                std::array<std::uint32_t, 2>
                    visualHeader{};

                std::memcpy(
                    visualHeader.data(),
                    data.data() +
                        static_cast<std::size_t>(
                            visualCursor),
                    sizeof(visualHeader));

                const std::uint32_t rawChunkId =
                    visualHeader[0];

                const std::uint32_t visualChunkId =
                    rawChunkId &
                    ~CompressionFlag;

                const bool compressed =
                    (rawChunkId &
                     CompressionFlag) !=
                    0u;

                const std::uint64_t visualDataOffset =
                    visualCursor +
                    sizeof(visualHeader);

                std::uint64_t visualChunkEnd = 0;

                if (!CheckedAdd(
                        visualDataOffset,
                        visualHeader[1],
                        visualChunkEnd) ||
                    visualChunkEnd >
                        visualEnd)
                {
                    error =
                        context +
                        " contains an out-of-bounds chunk.";

                    return false;
                }

                if (visualChunkId ==
                        OgfHeaderChunk &&
                    !headerFound)
                {
                    constexpr std::size_t HeaderSize =
                        44u;

                    if (compressed ||
                        visualHeader[1] <
                            HeaderSize)
                    {
                        error =
                            context +
                            " has an invalid OGF header.";

                        return false;
                    }

                    const std::byte* headerData =
                        data.data() +
                        static_cast<std::size_t>(
                            visualDataOffset);

                    visual.formatVersion =
                        std::to_integer<std::uint8_t>(
                            headerData[0]);

                    visual.type =
                        std::to_integer<std::uint8_t>(
                            headerData[1]);

                    std::memcpy(
                        &visual.shaderId,
                        headerData + 2,
                        sizeof(visual.shaderId));

                    std::memcpy(
                        &visual.boundsMinimum,
                        headerData + 4,
                        sizeof(visual.boundsMinimum));

                    std::memcpy(
                        &visual.boundsMaximum,
                        headerData + 16,
                        sizeof(visual.boundsMaximum));

                    if (visual.formatVersion !=
                        SupportedOgfVersion)
                    {
                        error =
                            context +
                            " uses unsupported OGF version " +
                            std::to_string(
                                visual.formatVersion) +
                            ".";

                        return false;
                    }

                    headerFound =
                        true;
                }
                else if (visualChunkId ==
                             OgfGeometryContainerChunk &&
                         !geometryFound)
                {
                    constexpr std::size_t GeometryReferenceSize =
                        6u *
                        sizeof(std::uint32_t);

                    if (compressed ||
                        visualHeader[1] <
                            GeometryReferenceSize)
                    {
                        error =
                            context +
                            " has an invalid geometry reference.";

                        return false;
                    }

                    std::array<std::uint32_t, 6>
                        geometry{};

                    std::memcpy(
                        geometry.data(),
                        data.data() +
                            static_cast<std::size_t>(
                                visualDataOffset),
                        sizeof(geometry));

                    visual.geometry.vertexBuffer =
                        geometry[0];

                    visual.geometry.vertexBase =
                        geometry[1];

                    visual.geometry.vertexCount =
                        geometry[2];

                    visual.geometry.indexBuffer =
                        geometry[3];

                    visual.geometry.indexBase =
                        geometry[4];

                    visual.geometry.indexCount =
                        geometry[5];

                    visual.geometry.valid =
                        true;

                    geometryFound =
                        true;
                }
                else if (visualChunkId ==
                             OgfChildrenLinksChunk &&
                         !childrenFound)
                {
                    if (compressed ||
                        visualHeader[1] <
                            sizeof(std::uint32_t))
                    {
                        error =
                            context +
                            " has an invalid hierarchy link chunk.";

                        return false;
                    }

                    std::uint32_t childCount = 0;

                    std::memcpy(
                        &childCount,
                        data.data() +
                            static_cast<std::size_t>(
                                visualDataOffset),
                        sizeof(childCount));

                    std::uint64_t linksSize = 0;

                    if (!CheckedMultiply(
                            childCount,
                            sizeof(std::uint32_t),
                            linksSize) ||
                        linksSize >
                            visualHeader[1] -
                                sizeof(std::uint32_t))
                    {
                        error =
                            context +
                            " hierarchy links exceed their chunk.";

                        return false;
                    }

                    visual.childVisuals.resize(
                        childCount);

                    if (linksSize !=
                        0u)
                    {
                        std::memcpy(
                            visual.childVisuals.data(),
                            data.data() +
                                static_cast<std::size_t>(
                                    visualDataOffset +
                                    sizeof(std::uint32_t)),
                            static_cast<std::size_t>(
                                linksSize));
                    }

                    childrenFound =
                        true;
                }

                visualCursor =
                    visualChunkEnd;
            }

            if (!headerFound)
            {
                error =
                    context +
                    " is missing required chunk " +
                    std::to_string(
                        OgfHeaderChunk) +
                    ".";

                return false;
            }

            output.visuals.push_back(
                std::move(
                    visual));

            cursor =
                visualEnd;
        }

        return true;
    }
}

namespace core::world::xray
{
    bool LevelLoader::Load(
        const std::filesystem::path& levelDirectory,
        LevelData& output,
        std::string& error) const
    {
        output =
            {};

        error.clear();

        const std::filesystem::path levelFile =
            levelDirectory /
            "level";

        const std::filesystem::path geometryFile =
            levelDirectory /
            "level.geom";

        BinaryFile level;

        if (!level.Open(
                levelFile,
                error))
        {
            return false;
        }

        BinaryFile geometry;

        if (!geometry.Open(
                geometryFile,
                error))
        {
            return false;
        }

        const Range levelRange
        {
            0u,
            level.Size()
        };

        const Range geometryRange
        {
            0u,
            geometry.Size()
        };

        Chunk levelHeader;

        if (!RequireChunk(
                level,
                levelRange,
                LevelHeaderChunk,
                levelHeader,
                error,
                "X-Ray level"))
        {
            return false;
        }

        if (levelHeader.data.size <
            4u)
        {
            error =
                "X-Ray level header is truncated.";

            return false;
        }

        std::array<std::uint16_t, 2>
            version{};

        if (!level.Read(
                levelHeader.data.offset,
                version.data(),
                sizeof(version),
                error))
        {
            return false;
        }

        if (version[0] !=
            SupportedLevelVersion)
        {
            error =
                "Unsupported X-Ray level version " +
                std::to_string(
                    version[0]) +
                "; expected version 14.";

            return false;
        }

        output.levelDirectory =
            levelDirectory;

        output.levelFile =
            levelFile;

        output.geometryFile =
            geometryFile;

        output.version =
            version[0];

        output.quality =
            version[1];

        Chunk shaders;

        if (!RequireChunk(
                level,
                levelRange,
                LevelShadersChunk,
                shaders,
                error,
                "X-Ray level"))
        {
            return false;
        }

        if (!ParseShaders(
                level,
                shaders,
                output,
                error))
        {
            return false;
        }

        Chunk vertexBuffers;

        if (!RequireChunk(
                geometry,
                geometryRange,
                LevelVertexBuffersChunk,
                vertexBuffers,
                error,
                "X-Ray level geometry"))
        {
            return false;
        }

        if (!ParseVertexBuffers(
                geometry,
                vertexBuffers,
                output,
                error))
        {
            return false;
        }

        Chunk indexBuffers;

        if (!RequireChunk(
                geometry,
                geometryRange,
                LevelIndexBuffersChunk,
                indexBuffers,
                error,
                "X-Ray level geometry"))
        {
            return false;
        }

        if (!ParseIndexBuffers(
                geometry,
                indexBuffers,
                output,
                error))
        {
            return false;
        }

        Chunk visuals;

        if (!RequireChunk(
                level,
                levelRange,
                LevelVisualsChunk,
                visuals,
                error,
                "X-Ray level"))
        {
            return false;
        }

        if (!ParseVisuals(
                level,
                visuals,
                output,
                error))
        {
            return false;
        }

        for (std::size_t visualIndex = 0;
             visualIndex <
                output.visuals.size();
             ++visualIndex)
        {
            const Visual& visual =
                output.visuals[
                    visualIndex];

            for (const std::uint32_t child :
                 visual.childVisuals)
            {
                if (child >=
                    output.visuals.size())
                {
                    error =
                        "X-Ray visual " +
                        std::to_string(
                            visualIndex) +
                        " references missing child visual " +
                        std::to_string(
                            child) +
                        ".";

                    return false;
                }
            }
        }

        return true;
    }
}
