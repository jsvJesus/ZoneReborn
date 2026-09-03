#include "Core/World/TerrainHeightDecoder.h"

#include "Core/Images/PngDecoder.h"
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
    constexpr std::uint32_t HeightMagic =
        0x00706D68u;

    constexpr std::uint32_t HeightVersion =
        4u;

    constexpr std::uint32_t QuantisedPngMagic =
        0x71706E67u;

    constexpr std::size_t HeightHeaderSize =
        32;

    constexpr float QuantisationLevel =
        0.001f;

    bool ReadUInt32(
        const std::span<const std::byte> data,
        const std::size_t offset,
        std::uint32_t& output) noexcept
    {
        if (offset > data.size() ||
            sizeof(std::uint32_t) >
                data.size() - offset)
        {
            return false;
        }

        output =
            static_cast<std::uint32_t>(
                std::to_integer<std::uint8_t>(
                    data[offset])) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[offset + 1]))
                << 8u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[offset + 2]))
                << 16u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[offset + 3]))
                << 24u
            );

        return true;
    }

    bool ReadFloat(
        const std::span<const std::byte> data,
        const std::size_t offset,
        float& output) noexcept
    {
        std::uint32_t raw = 0;

        if (!ReadUInt32(
                data,
                offset,
                raw))
        {
            return false;
        }

        std::memcpy(
            &output,
            &raw,
            sizeof(output));

        return true;
    }

    std::int32_t DecodeInt32(
        const std::byte* data) noexcept
    {
        const std::uint32_t value =
            static_cast<std::uint32_t>(
                std::to_integer<std::uint8_t>(
                    data[0])) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[1]))
                << 8u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[2]))
                << 16u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[3]))
                << 24u
            );

        std::int32_t result = 0;

        std::memcpy(
            &result,
            &value,
            sizeof(result));

        return result;
    }
}

namespace core::world
{
    bool TerrainHeightDecoder::Decode(
        const resources::ResourceFileSystem& resources,
        const std::string_view cdataLogicalPath,
        TerrainHeightData& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        std::vector<std::byte> cdata;

        if (!resources.ReadBinary(
                cdataLogicalPath,
                cdata))
        {
            error =
                "Unable to read cdata: " +
                std::string(
                    cdataLogicalPath);

            return false;
        }

        resources::ZipArchiveReader
            archiveReader;

        std::vector<std::byte>
            heightSection;

        if (!archiveReader.ExtractStored(
                std::span<const std::byte>(
                    cdata.data(),
                    cdata.size()),
                "terrain2/heights",
                heightSection,
                error))
        {
            error =
                "Unable to extract terrain2/heights from " +
                std::string(
                    cdataLogicalPath) +
                ": " +
                error;

            return false;
        }

        if (heightSection.size() <
            HeightHeaderSize +
                sizeof(std::uint32_t))
        {
            error =
                "Terrain height section is truncated.";

            return false;
        }

        const std::span<const std::byte>
            section(
                heightSection.data(),
                heightSection.size());

        std::uint32_t magic = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t compression = 0;
        std::uint32_t version = 0;

        float minHeight = 0.0f;
        float maxHeight = 0.0f;

        if (!ReadUInt32(
                section,
                0,
                magic) ||
            !ReadUInt32(
                section,
                4,
                width) ||
            !ReadUInt32(
                section,
                8,
                height) ||
            !ReadUInt32(
                section,
                12,
                compression) ||
            !ReadUInt32(
                section,
                16,
                version) ||
            !ReadFloat(
                section,
                20,
                minHeight) ||
            !ReadFloat(
                section,
                24,
                maxHeight))
        {
            error =
                "Terrain height header is truncated.";

            return false;
        }

        if (magic !=
            HeightMagic)
        {
            error =
                "Invalid terrain height magic.";

            return false;
        }

        if (version !=
            HeightVersion)
        {
            error =
                "Unsupported terrain height version: " +
                std::to_string(
                    version);

            return false;
        }

        if (compression != 0)
        {
            error =
                "Unsupported terrain height compression: " +
                std::to_string(
                    compression);

            return false;
        }

        if (width <= 4 ||
            height <= 4)
        {
            error =
                "Terrain height dimensions are invalid.";

            return false;
        }

        std::uint32_t qpngMagic = 0;

        if (!ReadUInt32(
                section,
                HeightHeaderSize,
                qpngMagic))
        {
            error =
                "Terrain qpng header is truncated.";

            return false;
        }

        if (qpngMagic !=
            QuantisedPngMagic)
        {
            error =
                "Terrain qpng magic is invalid.";

            return false;
        }

        const std::size_t pngOffset =
            HeightHeaderSize +
            sizeof(std::uint32_t);

        const std::span<const std::byte>
            pngData =
                section.subspan(
                    pngOffset);

        images::PngDecoder
            pngDecoder;

        images::RgbaImage
            image;

        if (!pngDecoder.Decode(
                pngData,
                image,
                error))
        {
            error =
                "Unable to decode terrain qpng: " +
                error;

            return false;
        }

        if (image.width != width ||
            image.height != height)
        {
            error =
                "Terrain PNG dimensions do not match height header.";

            return false;
        }

        const std::size_t expectedPixels =
            static_cast<std::size_t>(
                width) *
            height *
            4;

        if (image.pixels.size() !=
            expectedPixels)
        {
            error =
                "Terrain PNG pixel size is invalid.";

            return false;
        }

        TerrainHeightData result;

        result.width =
            width;

        result.height =
            height;

        result.visibleOffset =
            2;

        result.minHeight =
            minHeight;

        result.maxHeight =
            maxHeight;

        result.heights.resize(
            static_cast<std::size_t>(
                width) *
            height);

        for (std::size_t index = 0;
             index < result.heights.size();
             ++index)
        {
            const std::int32_t quantised =
                DecodeInt32(
                    image.pixels.data() +
                    index * 4);

            result.heights[index] =
                static_cast<float>(
                    quantised) *
                QuantisationLevel;
        }

        output =
            std::move(result);

        return true;
    }
}