#include "Core/World/TerrainLayerDecoder.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Images/PngDecoder.h"
#include "Core/Resources/ZipArchiveReader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::uint32_t BlendMagic =
        0x00646C62u;

    constexpr std::uint32_t RawBlendVersion =
        1u;

    constexpr std::uint32_t PngBlendVersion =
        2u;

    constexpr std::size_t BlendHeaderSize =
        64;

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

        std::memcpy(
            &output,
            data.data() + offset,
            sizeof(output));

        return true;
    }

    bool ReadFloat(
        const std::span<const std::byte> data,
        const std::size_t offset,
        float& output) noexcept
    {
        if (offset > data.size() ||
            sizeof(float) >
                data.size() - offset)
        {
            return false;
        }

        std::memcpy(
            &output,
            data.data() + offset,
            sizeof(output));

        return true;
    }

    bool ReadVector4(
        const std::span<const std::byte> data,
        const std::size_t offset,
        std::array<float, 4>& output) noexcept
    {
        for (std::size_t index = 0;
             index < output.size();
             ++index)
        {
            if (!ReadFloat(
                    data,
                    offset +
                        index *
                            sizeof(float),
                    output[index]))
            {
                return false;
            }
        }

        return true;
    }

    bool IsMissingEntryError(
        const std::string& error)
    {
        return error.starts_with(
            "ZIP entry was not found:");
    }
}

namespace core::world
{
    bool TerrainLayerDecoder::Decode(
        const resources::ResourceFileSystem& resources,
        const std::string_view cdataLogicalPath,
        std::vector<TerrainLayerData>& output,
        std::string& error) const
    {
        output.clear();
        error.clear();

        std::vector<std::byte> cdata;

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

        resources::ZipArchiveReader
            archiveReader;

        assets::TextureResolver
            textureResolver;

        images::PngDecoder
            pngDecoder;

        constexpr std::uint32_t MaximumLayers =
            64;

        for (std::uint32_t layerIndex = 1;
             layerIndex <= MaximumLayers;
             ++layerIndex)
        {
            const std::string sectionName =
                "terrain2/layer " +
                std::to_string(
                    layerIndex);

            std::vector<std::byte>
                sectionData;

            std::string archiveError;

            if (!archiveReader.ExtractStored(
                    std::span<const std::byte>(
                        cdata.data(),
                        cdata.size()),
                    sectionName,
                    sectionData,
                    archiveError))
            {
                if (IsMissingEntryError(
                        archiveError))
                {
                    break;
                }

                error =
                    "Unable to read " +
                    sectionName +
                    ": " +
                    archiveError;

                return false;
            }

            if (sectionData.size() <
                BlendHeaderSize +
                    sizeof(std::uint32_t))
            {
                error =
                    "Terrain layer is truncated: " +
                    sectionName;

                return false;
            }

            const std::span<const std::byte>
                data(
                    sectionData.data(),
                    sectionData.size());

            std::uint32_t magic = 0;
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            std::uint32_t bpp = 0;
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
                    bpp) ||
                !ReadUInt32(
                    data,
                    48,
                    version))
            {
                error =
                    "Unable to read terrain layer header: " +
                    sectionName;

                return false;
            }

            if (magic !=
                BlendMagic)
            {
                error =
                    "Invalid terrain layer magic: " +
                    sectionName;

                return false;
            }

            if (width == 0 ||
                height == 0)
            {
                error =
                    "Invalid terrain layer dimensions: " +
                    sectionName;

                return false;
            }

            if (bpp != 8)
            {
                error =
                    "Unsupported terrain layer bpp: " +
                    std::to_string(
                        bpp);

                return false;
            }

            if (version !=
                    RawBlendVersion &&
                version !=
                    PngBlendVersion)
            {
                error =
                    "Unsupported terrain layer version: " +
                    std::to_string(
                        version);

                return false;
            }

            TerrainLayerData layer;

            layer.sectionName =
                sectionName;

            layer.width =
                width;

            layer.height =
                height;

            if (!ReadVector4(
                    data,
                    16,
                    layer.uProjection) ||
                !ReadVector4(
                    data,
                    32,
                    layer.vProjection))
            {
                error =
                    "Unable to read terrain UV projection: " +
                    sectionName;

                return false;
            }

            std::uint32_t textureNameLength = 0;

            if (!ReadUInt32(
                    data,
                    BlendHeaderSize,
                    textureNameLength))
            {
                error =
                    "Unable to read terrain texture name length.";

                return false;
            }

            const std::size_t textureNameOffset =
                BlendHeaderSize +
                sizeof(std::uint32_t);

            if (textureNameOffset >
                    data.size() ||
                textureNameLength >
                    data.size() -
                        textureNameOffset)
            {
                error =
                    "Terrain texture name exceeds layer data.";

                return false;
            }

            std::string textureReference(
                reinterpret_cast<const char*>(
                    data.data() +
                    textureNameOffset),
                textureNameLength);

            if (const std::size_t zero =
                    textureReference.find('\0');
                zero !=
                    std::string::npos)
            {
                textureReference.resize(
                    zero);
            }

            if (textureReference.empty())
            {
                error =
                    "Terrain layer contains empty texture name.";

                return false;
            }

            if (!textureResolver.Resolve(
                    resources,
                    textureReference,
                    layer.texture))
            {
                error =
                    "Unable to resolve terrain texture: " +
                    textureReference;

                return false;
            }

            const std::size_t blendOffset =
                textureNameOffset +
                textureNameLength;

            if (blendOffset >
                data.size())
            {
                error =
                    "Terrain blend offset exceeds layer data.";

                return false;
            }

            const std::size_t pixelCount =
                static_cast<std::size_t>(
                    width) *
                static_cast<std::size_t>(
                    height);

            layer.blend.resize(
                pixelCount);

            if (version ==
                RawBlendVersion)
            {
                if (pixelCount >
                    data.size() -
                        blendOffset)
                {
                    error =
                        "Raw terrain blend map is truncated.";

                    return false;
                }

                std::memcpy(
                    layer.blend.data(),
                    data.data() +
                        blendOffset,
                    pixelCount);
            }
            else
            {
                const std::span<const std::byte>
                    pngData =
                        data.subspan(
                            blendOffset);

                images::RgbaImage image;

                std::string pngError;

                if (!pngDecoder.Decode(
                        pngData,
                        image,
                        pngError))
                {
                    error =
                        "Unable to decode terrain blend PNG: " +
                        pngError;

                    return false;
                }

                if (image.width !=
                        width ||
                    image.height !=
                        height)
                {
                    error =
                        "Terrain blend PNG dimensions do not match header.";

                    return false;
                }

                if (image.pixels.size() !=
                    pixelCount *
                        4)
                {
                    error =
                        "Terrain blend PNG pixel data is invalid.";

                    return false;
                }

                for (std::size_t pixel = 0;
                     pixel < pixelCount;
                     ++pixel)
                {
                    layer.blend[pixel] =
                        std::to_integer<std::uint8_t>(
                            image.pixels[
                                pixel * 4]);
                }
            }

            output.push_back(
                std::move(layer));
        }

        return true;
    }
}