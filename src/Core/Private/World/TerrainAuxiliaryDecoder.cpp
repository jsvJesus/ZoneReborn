#include "Core/World/TerrainAuxiliaryDecoder.h"

#include "Core/Compression/ZlibDecoder.h"
#include "Core/Resources/ZipArchiveReader.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::uint32_t PackedMagic =
        0x7A697000u;

    constexpr std::uint32_t PackedVersion =
        0x42AF9021u;

    constexpr std::uint32_t HoleMagic =
        0x006C6F68u;

    constexpr std::uint32_t ShadowMagic =
        0x00646873u;

    constexpr std::uint32_t DominantMagic =
        0x0074616Du;

    bool ReadUInt32(
        const std::span<const std::byte> data,
        const std::size_t offset,
        std::uint32_t& output) noexcept
    {
        if (offset >
                data.size() ||
            sizeof(std::uint32_t) >
                data.size() -
                    offset)
        {
            return false;
        }

        std::memcpy(
            &output,
            data.data() +
                offset,
            sizeof(output));

        return true;
    }

    bool IsMissingEntry(
        const std::string& error)
    {
        return
            error.starts_with(
                "ZIP entry was not found:");
    }

    bool TryExtract(
        const core::resources::ZipArchiveReader& reader,
        const std::span<const std::byte> archive,
        const std::string_view name,
        std::vector<std::byte>& output,
        bool& found,
        std::string& error)
    {
        output.clear();
        found =
            false;

        std::string readError;

        if (!reader.ExtractStored(
                archive,
                name,
                output,
                readError))
        {
            if (IsMissingEntry(
                    readError))
            {
                return true;
            }

            error =
                "Unable to extract " +
                std::string(
                    name) +
                ": " +
                readError;

            return false;
        }

        found =
            true;

        return true;
    }

    bool DecodePackedSection(
        const std::span<const std::byte> data,
        std::vector<std::byte>& output,
        std::string& error)
    {
        output.clear();

        if (data.size() <
            18)
        {
            error =
                "Packed terrain section is too small.";

            return false;
        }

        std::uint32_t magic = 0;
        std::uint32_t version = 0;
        std::uint32_t expectedSize = 0;

        if (!ReadUInt32(
                data,
                0,
                magic) ||
            !ReadUInt32(
                data,
                4,
                version) ||
            !ReadUInt32(
                data,
                8,
                expectedSize))
        {
            error =
                "Packed terrain section header is truncated.";

            return false;
        }

        if (magic !=
                PackedMagic ||
            version !=
                PackedVersion)
        {
            error =
                "Packed terrain section header is invalid.";

            return false;
        }

        core::compression::ZlibDecoder
            decoder;

        return decoder.Decode(
            data.subspan(
                12),
            expectedSize,
            output,
            error);
    }

    bool DecodeHoles(
        const std::span<const std::byte> encoded,
        core::world::TerrainHoleData& output,
        std::string& error)
    {
        output = {};

        std::vector<std::byte>
            decoded;

        if (!DecodePackedSection(
                encoded,
                decoded,
                error))
        {
            return false;
        }

        const std::span<const std::byte>
            data(
                decoded.data(),
                decoded.size());

        if (data.size() <
            16)
        {
            error =
                "Terrain holes section is truncated.";

            return false;
        }

        std::uint32_t magic = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t bitsPerPixel = 0;

        if (!ReadUInt32(
                data,
                0,
                magic) ||
            !ReadUInt32(
                data,
                4,
                width) ||
            !ReadUInt32(
                data,
                8,
                height) ||
            !ReadUInt32(
                data,
                12,
                bitsPerPixel))
        {
            error =
                "Terrain holes header is invalid.";

            return false;
        }

        if (magic !=
            HoleMagic)
        {
            error =
                "Terrain holes magic is invalid.";

            return false;
        }

        if (width == 0 ||
            height == 0 ||
            bitsPerPixel != 1)
        {
            error =
                "Terrain holes dimensions or format are invalid.";

            return false;
        }

        const std::size_t rowPitch =
            (
                (
                    static_cast<std::size_t>(
                        width) +
                    31u
                ) /
                32u
            ) *
            4u;

        const std::size_t requiredSize =
            16u +
            rowPitch *
                static_cast<std::size_t>(
                    height);

        if (data.size() <
            requiredSize)
        {
            error =
                "Terrain holes pixel data is truncated.";

            return false;
        }

        core::world::TerrainHoleData holes;

        holes.present =
            true;

        holes.width =
            width;

        holes.height =
            height;

        holes.cells.assign(
            static_cast<std::size_t>(
                width) *
                height,
            0);

        const std::byte* pixels =
            data.data() +
            16;

        for (std::uint32_t z = 0;
             z < height;
             ++z)
        {
            for (std::uint32_t x = 0;
                 x < width;
                 ++x)
            {
                const std::size_t byteOffset =
                    static_cast<std::size_t>(
                        z) *
                        rowPitch +
                    x /
                        8u;

                const std::uint8_t value =
                    std::to_integer<std::uint8_t>(
                        pixels[
                            byteOffset]);

                const bool hole =
                    (
                        value >>
                        (
                            x &
                            7u
                        )
                    ) &
                    1u;

                holes.cells[
                    static_cast<std::size_t>(
                        z) *
                        width +
                    x] =
                    hole
                        ? 1u
                        : 0u;
            }
        }

        output =
            std::move(
                holes);

        return true;
    }

    bool DecodeHorizonShadows(
        const std::span<const std::byte> encoded,
        core::world::TerrainHorizonShadowData& output,
        std::string& error)
    {
        output = {};

        std::vector<std::byte>
            decoded;

        if (!DecodePackedSection(
                encoded,
                decoded,
                error))
        {
            return false;
        }

        const std::span<const std::byte>
            data(
                decoded.data(),
                decoded.size());

        if (data.size() <
            32)
        {
            error =
                "Terrain horizon shadow data is truncated.";

            return false;
        }

        std::uint32_t magic = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t bitsPerPixel = 0;
        std::uint32_t version = 0;

        if (!ReadUInt32(
                data,
                0,
                magic) ||
            !ReadUInt32(
                data,
                4,
                width) ||
            !ReadUInt32(
                data,
                8,
                height) ||
            !ReadUInt32(
                data,
                12,
                bitsPerPixel) ||
            !ReadUInt32(
                data,
                16,
                version))
        {
            error =
                "Terrain horizon shadow header is invalid.";

            return false;
        }

        if (magic !=
                ShadowMagic ||
            width == 0 ||
            height == 0 ||
            bitsPerPixel != 32 ||
            version != 1)
        {
            error =
                "Unsupported terrain horizon shadow format.";

            return false;
        }

        const std::size_t count =
            static_cast<std::size_t>(
                width) *
            height;

        if (count >
            (
                data.size() -
                32
            ) /
            sizeof(std::uint32_t))
        {
            error =
                "Terrain horizon shadow pixels are truncated.";

            return false;
        }

        core::world::TerrainHorizonShadowData result;

        result.present =
            true;

        result.width =
            width;

        result.height =
            height;

        result.values.resize(
            count);

        for (std::size_t index = 0;
             index < count;
             ++index)
        {
            if (!ReadUInt32(
                    data,
                    32 +
                        index *
                            sizeof(std::uint32_t),
                    result.values[
                        index]))
            {
                error =
                    "Terrain horizon shadow data is truncated.";

                return false;
            }
        }

        output =
            std::move(
                result);

        return true;
    }

    bool DecodeDominantTextures(
        const std::span<const std::byte> data,
        core::world::TerrainDominantTextureData& output,
        std::string& error)
    {
        output = {};

        if (data.size() <
            32)
        {
            error =
                "Terrain dominant texture data is truncated.";

            return false;
        }

        std::uint32_t magic = 0;
        std::uint32_t version = 0;
        std::uint32_t textureCount = 0;
        std::uint32_t textureNameSize = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        if (!ReadUInt32(
                data,
                0,
                magic) ||
            !ReadUInt32(
                data,
                4,
                version) ||
            !ReadUInt32(
                data,
                8,
                textureCount) ||
            !ReadUInt32(
                data,
                12,
                textureNameSize) ||
            !ReadUInt32(
                data,
                16,
                width) ||
            !ReadUInt32(
                data,
                20,
                height))
        {
            error =
                "Terrain dominant texture header is invalid.";

            return false;
        }

        if (magic !=
                DominantMagic ||
            version !=
                1 ||
            textureCount == 0 ||
            textureNameSize == 0 ||
            width == 0 ||
            height == 0)
        {
            error =
                "Unsupported terrain dominant texture format.";

            return false;
        }

        const std::size_t namesSize =
            static_cast<std::size_t>(
                textureCount) *
            textureNameSize;

        if (namesSize >
            data.size() -
                32)
        {
            error =
                "Terrain dominant texture name table is truncated.";

            return false;
        }

        core::world::TerrainDominantTextureData
            result;

        result.present =
            true;

        result.width =
            width;

        result.height =
            height;

        result.textureReferences.reserve(
            textureCount);

        std::size_t offset =
            32;

        for (std::uint32_t index = 0;
             index < textureCount;
             ++index)
        {
            const char* name =
                reinterpret_cast<const char*>(
                    data.data() +
                    offset);

            std::size_t length =
                0;

            while (length <
                       textureNameSize &&
                   name[length] !=
                       '\0')
            {
                ++length;
            }

            result.textureReferences.emplace_back(
                name,
                length);

            offset +=
                textureNameSize;
        }

        std::vector<std::byte>
            indices;

        if (!DecodePackedSection(
                data.subspan(
                    offset),
                indices,
                error))
        {
            error =
                "Unable to decode terrain dominant texture map: " +
                error;

            return false;
        }

        const std::size_t expectedSize =
            static_cast<std::size_t>(
                width) *
            height;

        if (indices.size() !=
            expectedSize)
        {
            error =
                "Terrain dominant texture map size is invalid.";

            return false;
        }

        result.indices.resize(
            expectedSize);

        for (std::size_t index = 0;
             index < expectedSize;
             ++index)
        {
            const std::uint8_t value =
                std::to_integer<std::uint8_t>(
                    indices[
                        index]);

            if (value >=
                textureCount)
            {
                error =
                    "Terrain dominant texture map contains invalid index.";

                return false;
            }

            result.indices[
                index] =
                value;
        }

        output =
            std::move(
                result);

        return true;
    }
}

namespace core::world
{
    bool TerrainAuxiliaryDecoder::Decode(
        const resources::ResourceFileSystem& resources,
        const std::string_view cdataLogicalPath,
        TerrainAuxiliaryData& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        std::vector<std::byte>
            cdata;

        if (!resources.ReadBinary(
                cdataLogicalPath,
                cdata))
        {
            error =
                "Unable to read terrain cdata: " +
                std::string(
                    cdataLogicalPath);

            return false;
        }

        const std::span<const std::byte>
            archive(
                cdata.data(),
                cdata.size());

        resources::ZipArchiveReader
            reader;

        TerrainAuxiliaryData
            result;

        {
            std::vector<std::byte>
                section;

            bool found =
                false;

            if (!TryExtract(
                    reader,
                    archive,
                    "terrain2/holes",
                    section,
                    found,
                    error))
            {
                return false;
            }

            if (found &&
                !DecodeHoles(
                    section,
                    result.holes,
                    error))
            {
                return false;
            }
        }

        {
            std::vector<std::byte>
                section;

            bool found =
                false;

            if (!TryExtract(
                    reader,
                    archive,
                    "terrain2/horizonShadows",
                    section,
                    found,
                    error))
            {
                return false;
            }

            if (found &&
                !DecodeHorizonShadows(
                    section,
                    result.horizonShadows,
                    error))
            {
                return false;
            }
        }

        {
            std::vector<std::byte>
                section;

            bool found =
                false;

            if (!TryExtract(
                    reader,
                    archive,
                    "terrain2/dominantTextures",
                    section,
                    found,
                    error))
            {
                return false;
            }

            if (found &&
                !DecodeDominantTextures(
                    section,
                    result.dominantTextures,
                    error))
            {
                return false;
            }
        }

        {
            bool found =
                false;

            if (!TryExtract(
                    reader,
                    archive,
                    "terrain2/lodTexture.dds",
                    result.lodTextureDds,
                    found,
                    error))
            {
                return false;
            }

            if (!found)
            {
                result.lodTextureDds.clear();
            }
        }

        output =
            std::move(
                result);

        return true;
    }
}