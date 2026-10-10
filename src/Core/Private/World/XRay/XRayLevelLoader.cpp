#include "Core/World/XRay/XRayLevelLoader.h"
#include "Core/World/XRay/CformCollision.h"
#include "Core/World/XRay/XRaySpatialMath.h"

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
    constexpr std::uint32_t LevelPortalsChunk = 4u;
    constexpr std::uint32_t LevelSectorsChunk = 8u;
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

    bool ParseHom(
        const std::filesystem::path& directory,
        core::world::xray::LevelData& output,
        std::string& error)
    {
        const auto path = directory / "level.hom";
        std::error_code filesystemError;
        const bool exists = std::filesystem::exists(path, filesystemError);
        if (filesystemError)
        {
            error = "Unable to check level.hom: " + filesystemError.message();
            return false;
        }
        if (!exists)
            return true;

        BinaryFile file;
        if (!file.Open(path, error))
            return false;

        const Range range{0u, file.Size()};
        Chunk header, polygons;
        bool headerFound = false;
        bool polygonsFound = false;
        std::uint64_t cursor = 0;
        // Validate the entire stream, including chunks after the polygon data.
        while (cursor < range.size)
        {
            std::array<std::uint32_t, 2> rawHeader{};
            if (range.size - cursor < sizeof(rawHeader))
            {
                error = "level.hom contains a truncated chunk header.";
                return false;
            }
            if (!file.Read(cursor, rawHeader.data(), sizeof(rawHeader), error))
                return false;
            cursor += sizeof(rawHeader);
            if (rawHeader[1] > range.size - cursor)
            {
                error = "level.hom contains an out-of-bounds chunk.";
                return false;
            }
            const auto id = rawHeader[0] & ~CompressionFlag;
            if (id == 0u || id == 1u)
            {
                bool& found = id == 0u ? headerFound : polygonsFound;
                Chunk& chunk = id == 0u ? header : polygons;
                if (found)
                {
                    error = "level.hom contains a duplicate chunk " + std::to_string(id) + ".";
                    return false;
                }
                found = true;
                chunk.id = id;
                chunk.compressed = (rawHeader[0] & CompressionFlag) != 0u;
                chunk.data = {cursor, rawHeader[1]};
            }
            cursor += rawHeader[1];
        }
        if (headerFound)
        {
            std::uint32_t version = 0;
            if (header.compressed || header.data.size != sizeof(version))
            {
                error = "level.hom has an invalid version chunk.";
                return false;
            }
            if (!file.ReadValue(header.data.offset, version, error))
                return false;
            if (version != 0u)
            {
                error = "Unsupported level.hom version " + std::to_string(version) + ".";
                return false;
            }
        }

        if (!polygonsFound || polygons.compressed)
        {
            error = !polygonsFound
                ? "level.hom is missing polygon chunk 1."
                : "level.hom uses unsupported compressed polygon data.";
            return false;
        }

        using core::world::xray::HomTriangle;
        if (polygons.data.size % sizeof(HomTriangle) != 0u ||
            polygons.data.size > std::numeric_limits<std::size_t>::max())
        {
            error = "level.hom contains truncated polygon records.";
            return false;
        }

        output.homTriangles.resize(
            static_cast<std::size_t>(polygons.data.size / sizeof(HomTriangle)));
        if (!file.Read(polygons.data.offset, output.homTriangles.data(),
                static_cast<std::size_t>(polygons.data.size), error))
            return false;

        for (const auto& triangle : output.homTriangles)
        {
            for (const auto vertex : triangle.vertices)
            {
                if (!core::world::xray::spatial::Finite(vertex))
                {
                    error = "level.hom contains a non-finite polygon vertex.";
                    return false;
                }
            }
        }

        output.homPresent = true;
        return true;
    }

    bool ParseTopology(
        BinaryFile& file, const Range& range,
        core::world::xray::LevelData& output, std::string& error)
    {
        using namespace core::world::xray;
        Chunk portals, sectors;
        if (!RequireChunk(file, range, LevelPortalsChunk, portals, error, "X-Ray portals") ||
            !RequireChunk(file, range, LevelSectorsChunk, sectors, error, "X-Ray sectors")) return false;
        constexpr std::uint64_t PortalSize = 80u;
        if (portals.data.size % PortalSize != 0u || portals.data.size / PortalSize > 65536u)
        { error = "X-Ray portal array has an invalid size."; return false; }
        output.portals.resize(static_cast<std::size_t>(portals.data.size / PortalSize));
        for (std::size_t i = 0; i < output.portals.size(); ++i)
        {
            std::array<std::byte, PortalSize> bytes{};
            if (!file.Read(portals.data.offset + i * PortalSize, bytes.data(), bytes.size(), error)) return false;
            auto& p = output.portals[i];
            std::memcpy(&p.frontSector, bytes.data(), 2u);
            std::memcpy(&p.backSector, bytes.data() + 2u, 2u);
            std::memcpy(p.vertices.data(), bytes.data() + 4u, 72u);
            std::memcpy(&p.vertexCount, bytes.data() + 76u, 4u);
            if (p.vertexCount < 3u || p.vertexCount > 6u)
            { error = "X-Ray portal has an invalid vertex count."; return false; }
            for (std::uint32_t v = 0; v < p.vertexCount; ++v)
                if (!spatial::Finite(p.vertices[v]))
                { error = "X-Ray portal contains non-finite coordinates."; return false; }
        }
        std::uint64_t cursor = sectors.data.offset;
        const std::uint64_t end = cursor + sectors.data.size;
        while (cursor < end)
        {
            std::array<std::uint32_t, 2> header{};
            if (end - cursor < 8u || !file.Read(cursor, header.data(), 8u, error))
            { error = "X-Ray sector chunk header is truncated."; return false; }
            cursor += 8u;
            if (header[0] != output.sectors.size() || header[1] > end - cursor ||
                output.sectors.size() >= 65536u)
            { error = "X-Ray sector chunk ID or range is invalid."; return false; }
            const Range sectorRange{cursor, header[1]};
            Chunk root, links;
            if (!RequireChunk(file, sectorRange, 2u, root, error, "X-Ray sector root") ||
                !RequireChunk(file, sectorRange, 1u, links, error, "X-Ray sector portals")) return false;
            if (root.data.size != 4u || links.data.size % 2u != 0u)
            { error = "X-Ray sector payload size is invalid."; return false; }
            Sector sector;
            if (!file.ReadValue(root.data.offset, sector.rootVisual, error)) return false;
            if (sector.rootVisual >= output.visuals.size())
            { error = "X-Ray sector root references a missing visual."; return false; }
            sector.portals.resize(static_cast<std::size_t>(links.data.size / 2u));
            if (!sector.portals.empty() && !file.Read(links.data.offset, sector.portals.data(),
                sector.portals.size() * 2u, error)) return false;
            for (const auto id : sector.portals)
            {
                if (id >= output.portals.size())
                { error = "X-Ray sector references a missing portal."; return false; }
                const auto& p = output.portals[id];
                if (p.frontSector != output.sectors.size() && p.backSector != output.sectors.size())
                { error = "X-Ray portal does not connect its owning sector."; return false; }
            }
            output.sectors.push_back(std::move(sector));
            cursor += header[1];
        }
        if (output.sectors.empty()) { error = "X-Ray level has no sectors."; return false; }
        for (const auto& p : output.portals)
            if (p.frontSector >= output.sectors.size() || p.backSector >= output.sectors.size() ||
                p.frontSector == p.backSector)
            { error = "X-Ray portal sector indices are invalid."; return false; }
        return true;
    }

    bool ParseSlideWindows(BinaryFile& file, const Range& range,
        core::world::xray::LevelData& output, std::string& error)
    {
        Chunk chunk; bool found = false;
        if (!FindChunk(file, range, 11u, chunk, found, error, "X-Ray slide windows")) return false;
        if (!found) return true;
        if (chunk.compressed || chunk.data.size < 4u)
        { error = "X-Ray slide-window chunk is invalid."; return false; }
        std::uint32_t count = 0;
        if (!file.ReadValue(chunk.data.offset, count, error)) return false;
        if (count > (chunk.data.size - 4u) / 20u)
        { error = "X-Ray slide-window container count exceeds its chunk."; return false; }
        output.slideWindows.resize(count);
        std::uint64_t cursor = chunk.data.offset + 4u;
        const auto end = chunk.data.offset + chunk.data.size;
        for (auto& windows : output.slideWindows)
        {
            std::array<std::uint32_t, 5> header{};
            if (end - cursor < 20u || !file.Read(cursor, header.data(), 20u, error)) return false;
            cursor += 20u;
            const auto windowCount = header[4];
            if (windowCount > (end - cursor) / 8u)
            { error = "X-Ray slide-window records exceed their chunk."; return false; }
            windows.resize(windowCount);
            static_assert(sizeof(core::world::xray::SlideWindow) == 8u);
            if (!windows.empty() && !file.Read(cursor, windows.data(), windows.size() * 8u, error)) return false;
            cursor += windows.size() * 8u;
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
                             11u)
                {
                    constexpr std::size_t VertexSize = 28u;
                    if (compressed || visualHeader[1] != 32u * VertexSize ||
                        visual.lodIndex != core::world::xray::InvalidIndex)
                    { error = context + " has invalid LOD facets."; return false; }
                    core::world::xray::LodDefinition lod;
                    const auto* source = data.data() + static_cast<std::size_t>(visualDataOffset);
                    for (std::size_t v = 0; v < lod.vertices.size(); ++v)
                    {
                        auto& target = lod.vertices[v];
                        std::memcpy(&target.position, source + v * VertexSize, 12u);
                        std::memcpy(&target.u, source + v * VertexSize + 12u, 4u);
                        std::memcpy(&target.v, source + v * VertexSize + 16u, 4u);
                        std::memcpy(&target.colour, source + v * VertexSize + 20u, 4u);
                        target.sun = std::to_integer<std::uint8_t>(source[v * VertexSize + 24u]);
                        if (!core::world::xray::spatial::Finite(target.position) ||
                            !std::isfinite(target.u) || !std::isfinite(target.v))
                        { error = context + " has non-finite LOD vertices."; return false; }
                    }
                    visual.lodIndex = static_cast<std::uint32_t>(output.lods.size());
                    output.lods.push_back(std::move(lod));
                }
                else if (visualChunkId == 12u)
                {
                    if (compressed || visualHeader[1] != 104u ||
                        visual.treeIndex != core::world::xray::InvalidIndex)
                    { error = context + " has an invalid tree definition."; return false; }
                    std::array<float, 26> values{};
                    std::memcpy(values.data(), data.data() + static_cast<std::size_t>(visualDataOffset), 104u);
                    for (const float value : values)
                        if (!std::isfinite(value))
                        { error = context + " has a non-finite tree transform/lighting."; return false; }
                    core::world::xray::TreeDefinition tree;
                    constexpr std::array<std::size_t, 12> MatrixElements{0,1,2,4,5,6,8,9,10,12,13,14};
                    for (std::size_t i = 0; i < MatrixElements.size(); ++i)
                        tree.transform.values[i] = values[MatrixElements[i]];
                    for (std::size_t i = 0; i < 5u; ++i)
                    { tree.lightingScale[i] = values[16u+i]*0.5f; tree.lightingBias[i] = values[21u+i]*0.5f; }
                    visual.treeIndex = static_cast<std::uint32_t>(output.trees.size());
                    output.trees.push_back(tree);
                }
                else if (visualChunkId == 20u)
                {
                    if (compressed || visualHeader[1] != 4u)
                    { error = context + " has an invalid slide-window reference."; return false; }
                    std::memcpy(&visual.slideWindowIndex,
                        data.data() + static_cast<std::size_t>(visualDataOffset), 4u);
                }
                else if (visualChunkId == 22u)
                {
                    if (compressed) { error = context + " has compressed fast geometry."; return false; }
                    std::uint64_t fastCursor = visualDataOffset;
                    while (fastCursor < visualChunkEnd)
                    {
                        std::array<std::uint32_t, 2> fastHeader{};
                        if (visualChunkEnd - fastCursor < 8u)
                        { error = context + " has truncated fast-geometry chunks."; return false; }
                        std::memcpy(fastHeader.data(), data.data() + static_cast<std::size_t>(fastCursor), 8u);
                        fastCursor += 8u;
                        if (fastHeader[1] > visualChunkEnd - fastCursor || (fastHeader[0] & CompressionFlag))
                        { error = context + " has invalid fast-geometry chunks."; return false; }
                        if (fastHeader[0] == OgfGeometryContainerChunk)
                        {
                            if (fastHeader[1] != 24u)
                            { error = context + " has an invalid fast-geometry reference."; return false; }
                            std::array<std::uint32_t, 6> reference{};
                            std::memcpy(reference.data(), data.data() + static_cast<std::size_t>(fastCursor), 24u);
                            visual.fastGeometry = {reference[0], reference[1], reference[2],
                                reference[3], reference[4], reference[5], true};
                        }
                        fastCursor += fastHeader[1];
                    }
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

            const core::world::xray::spatial::Bounds bounds{visual.boundsMinimum, visual.boundsMaximum};
            if (!bounds.Valid() ||
                (visual.type == 6u && visual.lodIndex == core::world::xray::InvalidIndex) ||
                ((visual.type == 7u || visual.type == 11u) && visual.treeIndex == core::world::xray::InvalidIndex))
            { error = context + " has invalid bounds or missing type-specific metadata."; return false; }

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

        if (!ParseSlideWindows(geometry, geometryRange, output, error)) return false;

        const auto geomxPath = levelDirectory / "level.geomx";
        std::error_code existsError;
        const bool geomxExists = std::filesystem::exists(geomxPath, existsError);
        if (existsError) { error = "Unable to check level.geomx: " + existsError.message(); return false; }
        if (geomxExists)
        {
            BinaryFile geomx;
            if (!geomx.Open(geomxPath, error)) return false;
            const Range geomxRange{0u, geomx.Size()};
            Chunk vb, ib;
            core::world::xray::LevelData descriptors;
            if (!RequireChunk(geomx, geomxRange, LevelVertexBuffersChunk, vb, error, "X-Ray geomx") ||
                !RequireChunk(geomx, geomxRange, LevelIndexBuffersChunk, ib, error, "X-Ray geomx") ||
                !ParseVertexBuffers(geomx, vb, descriptors, error) ||
                !ParseIndexBuffers(geomx, ib, descriptors, error)) return false;
            output.secondaryGeometryFile = geomxPath;
            output.secondaryVertexBuffers = std::move(descriptors.vertexBuffers);
            output.secondaryIndexBuffers = std::move(descriptors.indexBuffers);
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

            if (visual.slideWindowIndex != InvalidIndex && visual.slideWindowIndex >= output.slideWindows.size())
            { error = "X-Ray visual references a missing slide-window container."; return false; }

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

        // Iterative tri-colour graph validation: no recursion on a 450k-node map.
        std::vector<std::uint8_t> colours(output.visuals.size(), 0u);
        struct Visit { std::uint32_t id; std::size_t next; };
        std::vector<Visit> stack;
        for (std::uint32_t root = 0; root < output.visuals.size(); ++root)
        {
            if (colours[root] != 0u) continue;
            stack.push_back({root, 0u}); colours[root] = 1u;
            while (!stack.empty())
            {
                auto& visit = stack.back();
                const auto& children = output.visuals[visit.id].childVisuals;
                if (visit.next == children.size())
                { colours[visit.id] = 2u; stack.pop_back(); continue; }
                const auto child = children[visit.next++];
                if (colours[child] == 1u) { error = "X-Ray visual hierarchy contains a cycle."; return false; }
                if (colours[child] == 0u) { colours[child] = 1u; stack.push_back({child, 0u}); }
            }
        }
        if (!ParseTopology(level, levelRange, output, error)) return false;
        if (!ParseHom(levelDirectory, output, error)) return false;

        const auto collisionFile = levelDirectory / "level.cform";
        std::error_code collisionError;
        const bool collisionPresent = std::filesystem::exists(collisionFile, collisionError);
        if (collisionError)
        {
            error = "Unable to inspect level.cform: " + collisionError.message();
            return false;
        }
        if (collisionPresent)
        {
            auto collision = std::make_shared<CformCollision>();
            if (!collision->Load(collisionFile, output.sectors.size(), error)) return false;
            output.collision = std::move(collision);
        }

        return true;
    }
}
