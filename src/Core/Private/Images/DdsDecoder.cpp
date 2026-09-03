#include "Core/Images/DdsDecoder.h"

#include "Core/Images/WicImageDecoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>

namespace
{
    constexpr std::uint32_t DdsMagic =
        0x20534444u;

    constexpr std::uint32_t HeaderSize =
        124u;

    constexpr std::uint32_t PixelFormatSize =
        32u;

    constexpr std::size_t DataOffset =
        128u;

    constexpr std::uint32_t DdsdPitch =
        0x00000008u;

    constexpr std::uint32_t DdpfAlphaPixels =
        0x00000001u;

    constexpr std::uint32_t DdpfAlpha =
        0x00000002u;

    constexpr std::uint32_t DdpfFourCC =
        0x00000004u;

    constexpr std::uint32_t DdpfRgb =
        0x00000040u;

    constexpr std::uint32_t DdpfLuminance =
        0x00020000u;

    constexpr std::uint32_t MakeFourCC(
        const char a,
        const char b,
        const char c,
        const char d) noexcept
    {
        return
            static_cast<std::uint32_t>(
                static_cast<unsigned char>(a)) |
            (
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(b))
                << 8u
            ) |
            (
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(c))
                << 16u
            ) |
            (
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(d))
                << 24u
            );
    }

    constexpr std::uint32_t FourCcDxt1 =
        MakeFourCC(
            'D',
            'X',
            'T',
            '1');

    constexpr std::uint32_t FourCcDxt3 =
        MakeFourCC(
            'D',
            'X',
            'T',
            '3');

    constexpr std::uint32_t FourCcDxt5 =
        MakeFourCC(
            'D',
            'X',
            'T',
            '5');

    struct Colour final
    {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;
    };

    bool ReadUInt16(
        const std::span<const std::byte> data,
        const std::size_t offset,
        std::uint16_t& output) noexcept
    {
        if (offset >
                data.size() ||
            sizeof(std::uint16_t) >
                data.size() -
                    offset)
        {
            return false;
        }

        output =
            static_cast<std::uint16_t>(
                std::to_integer<std::uint8_t>(
                    data[offset])) |

            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(
                    std::to_integer<std::uint8_t>(
                        data[offset + 1]))
                << 8u);

        return true;
    }

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

    std::uint64_t ReadUInt48(
        const std::byte* data) noexcept
    {
        std::uint64_t output = 0;

        for (std::uint32_t index = 0;
             index < 6;
             ++index)
        {
            output |=
                static_cast<std::uint64_t>(
                    std::to_integer<std::uint8_t>(
                        data[index]))
                <<
                (
                    index *
                    8u
                );
        }

        return output;
    }

    std::uint64_t ReadUInt64(
        const std::byte* data) noexcept
    {
        std::uint64_t output = 0;

        for (std::uint32_t index = 0;
             index < 8;
             ++index)
        {
            output |=
                static_cast<std::uint64_t>(
                    std::to_integer<std::uint8_t>(
                        data[index]))
                <<
                (
                    index *
                    8u
                );
        }

        return output;
    }

    Colour Decode565(
        const std::uint16_t value) noexcept
    {
        Colour colour;

        const std::uint32_t r =
            (
                value >>
                11u
            ) &
            0x1Fu;

        const std::uint32_t g =
            (
                value >>
                5u
            ) &
            0x3Fu;

        const std::uint32_t b =
            value &
            0x1Fu;

        colour.r =
            static_cast<std::uint8_t>(
                (
                    r *
                    255u +
                    15u
                ) /
                31u);

        colour.g =
            static_cast<std::uint8_t>(
                (
                    g *
                    255u +
                    31u
                ) /
                63u);

        colour.b =
            static_cast<std::uint8_t>(
                (
                    b *
                    255u +
                    15u
                ) /
                31u);

        colour.a =
            255;

        return colour;
    }

    Colour Interpolate(
        const Colour& first,
        const Colour& second,
        const std::uint32_t firstWeight,
        const std::uint32_t secondWeight,
        const std::uint32_t divisor) noexcept
    {
        Colour output;

        output.r =
            static_cast<std::uint8_t>(
                (
                    static_cast<std::uint32_t>(
                        first.r) *
                        firstWeight +
                    static_cast<std::uint32_t>(
                        second.r) *
                        secondWeight
                ) /
                divisor);

        output.g =
            static_cast<std::uint8_t>(
                (
                    static_cast<std::uint32_t>(
                        first.g) *
                        firstWeight +
                    static_cast<std::uint32_t>(
                        second.g) *
                        secondWeight
                ) /
                divisor);

        output.b =
            static_cast<std::uint8_t>(
                (
                    static_cast<std::uint32_t>(
                        first.b) *
                        firstWeight +
                    static_cast<std::uint32_t>(
                        second.b) *
                        secondWeight
                ) /
                divisor);

        output.a =
            static_cast<std::uint8_t>(
                (
                    static_cast<std::uint32_t>(
                        first.a) *
                        firstWeight +
                    static_cast<std::uint32_t>(
                        second.a) *
                        secondWeight
                ) /
                divisor);

        return output;
    }

    void BuildColourPalette(
        const std::uint16_t colour0,
        const std::uint16_t colour1,
        const bool allowTransparent,
        std::array<Colour, 4>& palette) noexcept
    {
        palette[0] =
            Decode565(
                colour0);

        palette[1] =
            Decode565(
                colour1);

        if (allowTransparent &&
            colour0 <=
                colour1)
        {
            palette[2] =
                Interpolate(
                    palette[0],
                    palette[1],
                    1,
                    1,
                    2);

            palette[3] =
            {
                0,
                0,
                0,
                0
            };

            return;
        }

        palette[2] =
            Interpolate(
                palette[0],
                palette[1],
                2,
                1,
                3);

        palette[3] =
            Interpolate(
                palette[0],
                palette[1],
                1,
                2,
                3);
    }

    void CreateImage(
        const std::uint32_t width,
        const std::uint32_t height,
        core::images::RgbaImage& output)
    {
        output = {};

        output.width =
            width;

        output.height =
            height;

        output.pixels.assign(
            static_cast<std::size_t>(
                width) *
                height *
                4,
            std::byte{0});
    }

    void WritePixel(
        core::images::RgbaImage& image,
        const std::uint32_t x,
        const std::uint32_t y,
        const Colour colour) noexcept
    {
        if (x >=
                image.width ||
            y >=
                image.height)
        {
            return;
        }

        const std::size_t offset =
            (
                static_cast<std::size_t>(
                    y) *
                    image.width +
                x
            ) *
            4;

        image.pixels[
            offset + 0] =
            static_cast<std::byte>(
                colour.r);

        image.pixels[
            offset + 1] =
            static_cast<std::byte>(
                colour.g);

        image.pixels[
            offset + 2] =
            static_cast<std::byte>(
                colour.b);

        image.pixels[
            offset + 3] =
            static_cast<std::byte>(
                colour.a);
    }

    bool DecodeBc1(
        const std::span<const std::byte> data,
        const std::uint32_t width,
        const std::uint32_t height,
        core::images::RgbaImage& output,
        std::string& error)
    {
        const std::uint32_t blocksX =
            (
                width +
                3u
            ) /
            4u;

        const std::uint32_t blocksY =
            (
                height +
                3u
            ) /
            4u;

        const std::size_t requiredSize =
            static_cast<std::size_t>(
                blocksX) *
            blocksY *
            8;

        if (data.size() <
            requiredSize)
        {
            error =
                "DDS BC1 data is truncated.";

            return false;
        }

        CreateImage(
            width,
            height,
            output);

        std::size_t blockOffset = 0;

        for (std::uint32_t blockY = 0;
             blockY < blocksY;
             ++blockY)
        {
            for (std::uint32_t blockX = 0;
                 blockX < blocksX;
                 ++blockX)
            {
                std::uint16_t colour0 = 0;
                std::uint16_t colour1 = 0;

                if (!ReadUInt16(
                        data,
                        blockOffset + 0,
                        colour0) ||
                    !ReadUInt16(
                        data,
                        blockOffset + 2,
                        colour1))
                {
                    error =
                        "DDS BC1 colour block is truncated.";

                    return false;
                }

                std::uint32_t indices = 0;

                if (!ReadUInt32(
                        data,
                        blockOffset + 4,
                        indices))
                {
                    error =
                        "DDS BC1 index block is truncated.";

                    return false;
                }

                std::array<Colour, 4>
                    palette{};

                BuildColourPalette(
                    colour0,
                    colour1,
                    true,
                    palette);

                for (std::uint32_t pixelY = 0;
                     pixelY < 4;
                     ++pixelY)
                {
                    for (std::uint32_t pixelX = 0;
                         pixelX < 4;
                         ++pixelX)
                    {
                        const std::uint32_t pixel =
                            pixelY *
                                4u +
                            pixelX;

                        const std::uint32_t paletteIndex =
                            (
                                indices >>
                                (
                                    pixel *
                                    2u
                                )
                            ) &
                            0x3u;

                        WritePixel(
                            output,
                            blockX *
                                4u +
                                pixelX,
                            blockY *
                                4u +
                                pixelY,
                            palette[
                                paletteIndex]);
                    }
                }

                blockOffset +=
                    8;
            }
        }

        return true;
    }

    bool DecodeBc2(
        const std::span<const std::byte> data,
        const std::uint32_t width,
        const std::uint32_t height,
        core::images::RgbaImage& output,
        std::string& error)
    {
        const std::uint32_t blocksX =
            (
                width +
                3u
            ) /
            4u;

        const std::uint32_t blocksY =
            (
                height +
                3u
            ) /
            4u;

        const std::size_t requiredSize =
            static_cast<std::size_t>(
                blocksX) *
            blocksY *
            16;

        if (data.size() <
            requiredSize)
        {
            error =
                "DDS BC2 data is truncated.";

            return false;
        }

        CreateImage(
            width,
            height,
            output);

        std::size_t blockOffset = 0;

        for (std::uint32_t blockY = 0;
             blockY < blocksY;
             ++blockY)
        {
            for (std::uint32_t blockX = 0;
                 blockX < blocksX;
                 ++blockX)
            {
                const std::uint64_t alphaBits =
                    ReadUInt64(
                        data.data() +
                        blockOffset);

                const std::span<const std::byte>
                    colourBlock =
                        data.subspan(
                            blockOffset +
                                8,
                            8);

                std::uint16_t colour0 = 0;
                std::uint16_t colour1 = 0;

                std::uint32_t indices = 0;

                if (!ReadUInt16(
                        colourBlock,
                        0,
                        colour0) ||
                    !ReadUInt16(
                        colourBlock,
                        2,
                        colour1) ||
                    !ReadUInt32(
                        colourBlock,
                        4,
                        indices))
                {
                    error =
                        "DDS BC2 block is truncated.";

                    return false;
                }

                std::array<Colour, 4>
                    palette{};

                BuildColourPalette(
                    colour0,
                    colour1,
                    false,
                    palette);

                for (std::uint32_t pixelY = 0;
                     pixelY < 4;
                     ++pixelY)
                {
                    for (std::uint32_t pixelX = 0;
                         pixelX < 4;
                         ++pixelX)
                    {
                        const std::uint32_t pixel =
                            pixelY *
                                4u +
                            pixelX;

                        const std::uint32_t paletteIndex =
                            (
                                indices >>
                                (
                                    pixel *
                                    2u
                                )
                            ) &
                            0x3u;

                        Colour colour =
                            palette[
                                paletteIndex];

                        const std::uint8_t alpha =
                            static_cast<std::uint8_t>(
                                (
                                    alphaBits >>
                                    (
                                        pixel *
                                        4u
                                    )
                                ) &
                                0x0Fu);

                        colour.a =
                            static_cast<std::uint8_t>(
                                alpha *
                                17u);

                        WritePixel(
                            output,
                            blockX *
                                4u +
                                pixelX,
                            blockY *
                                4u +
                                pixelY,
                            colour);
                    }
                }

                blockOffset +=
                    16;
            }
        }

        return true;
    }

    void BuildBc3AlphaPalette(
        const std::uint8_t alpha0,
        const std::uint8_t alpha1,
        std::array<std::uint8_t, 8>& palette) noexcept
    {
        palette[0] =
            alpha0;

        palette[1] =
            alpha1;

        if (alpha0 >
            alpha1)
        {
            palette[2] =
                static_cast<std::uint8_t>(
                    (
                        6u *
                            alpha0 +
                        1u *
                            alpha1
                    ) /
                    7u);

            palette[3] =
                static_cast<std::uint8_t>(
                    (
                        5u *
                            alpha0 +
                        2u *
                            alpha1
                    ) /
                    7u);

            palette[4] =
                static_cast<std::uint8_t>(
                    (
                        4u *
                            alpha0 +
                        3u *
                            alpha1
                    ) /
                    7u);

            palette[5] =
                static_cast<std::uint8_t>(
                    (
                        3u *
                            alpha0 +
                        4u *
                            alpha1
                    ) /
                    7u);

            palette[6] =
                static_cast<std::uint8_t>(
                    (
                        2u *
                            alpha0 +
                        5u *
                            alpha1
                    ) /
                    7u);

            palette[7] =
                static_cast<std::uint8_t>(
                    (
                        1u *
                            alpha0 +
                        6u *
                            alpha1
                    ) /
                    7u);

            return;
        }

        palette[2] =
            static_cast<std::uint8_t>(
                (
                    4u *
                        alpha0 +
                    1u *
                        alpha1
                ) /
                5u);

        palette[3] =
            static_cast<std::uint8_t>(
                (
                    3u *
                        alpha0 +
                    2u *
                        alpha1
                ) /
                5u);

        palette[4] =
            static_cast<std::uint8_t>(
                (
                    2u *
                        alpha0 +
                    3u *
                        alpha1
                ) /
                5u);

        palette[5] =
            static_cast<std::uint8_t>(
                (
                    1u *
                        alpha0 +
                    4u *
                        alpha1
                ) /
                5u);

        palette[6] =
            0;

        palette[7] =
            255;
    }

    bool DecodeBc3(
        const std::span<const std::byte> data,
        const std::uint32_t width,
        const std::uint32_t height,
        core::images::RgbaImage& output,
        std::string& error)
    {
        const std::uint32_t blocksX =
            (
                width +
                3u
            ) /
            4u;

        const std::uint32_t blocksY =
            (
                height +
                3u
            ) /
            4u;

        const std::size_t requiredSize =
            static_cast<std::size_t>(
                blocksX) *
            blocksY *
            16;

        if (data.size() <
            requiredSize)
        {
            error =
                "DDS BC3 data is truncated.";

            return false;
        }

        CreateImage(
            width,
            height,
            output);

        std::size_t blockOffset = 0;

        for (std::uint32_t blockY = 0;
             blockY < blocksY;
             ++blockY)
        {
            for (std::uint32_t blockX = 0;
                 blockX < blocksX;
                 ++blockX)
            {
                const std::uint8_t alpha0 =
                    std::to_integer<std::uint8_t>(
                        data[
                            blockOffset +
                            0]);

                const std::uint8_t alpha1 =
                    std::to_integer<std::uint8_t>(
                        data[
                            blockOffset +
                            1]);

                const std::uint64_t alphaIndices =
                    ReadUInt48(
                        data.data() +
                        blockOffset +
                        2);

                std::array<std::uint8_t, 8>
                    alphaPalette{};

                BuildBc3AlphaPalette(
                    alpha0,
                    alpha1,
                    alphaPalette);

                const std::span<const std::byte>
                    colourBlock =
                        data.subspan(
                            blockOffset +
                                8,
                            8);

                std::uint16_t colour0 = 0;
                std::uint16_t colour1 = 0;

                std::uint32_t colourIndices = 0;

                if (!ReadUInt16(
                        colourBlock,
                        0,
                        colour0) ||
                    !ReadUInt16(
                        colourBlock,
                        2,
                        colour1) ||
                    !ReadUInt32(
                        colourBlock,
                        4,
                        colourIndices))
                {
                    error =
                        "DDS BC3 colour block is truncated.";

                    return false;
                }

                std::array<Colour, 4>
                    colourPalette{};

                BuildColourPalette(
                    colour0,
                    colour1,
                    false,
                    colourPalette);

                for (std::uint32_t pixelY = 0;
                     pixelY < 4;
                     ++pixelY)
                {
                    for (std::uint32_t pixelX = 0;
                         pixelX < 4;
                         ++pixelX)
                    {
                        const std::uint32_t pixel =
                            pixelY *
                                4u +
                            pixelX;

                        const std::uint32_t colourIndex =
                            (
                                colourIndices >>
                                (
                                    pixel *
                                    2u
                                )
                            ) &
                            0x3u;

                        const std::uint32_t alphaIndex =
                            static_cast<std::uint32_t>(
                                (
                                    alphaIndices >>
                                    (
                                        pixel *
                                        3u
                                    )
                                ) &
                                0x7u);

                        Colour colour =
                            colourPalette[
                                colourIndex];

                        colour.a =
                            alphaPalette[
                                alphaIndex];

                        WritePixel(
                            output,
                            blockX *
                                4u +
                                pixelX,
                            blockY *
                                4u +
                                pixelY,
                            colour);
                    }
                }

                blockOffset +=
                    16;
            }
        }

        return true;
    }

    std::uint8_t DecodeMaskedChannel(
        const std::uint32_t pixel,
        const std::uint32_t mask,
        const std::uint8_t defaultValue) noexcept
    {
        if (mask == 0)
        {
            return defaultValue;
        }

        std::uint32_t shift = 0;
        std::uint32_t shiftedMask =
            mask;

        while (
            (
                shiftedMask &
                1u
            ) ==
            0u)
        {
            shiftedMask >>=
                1u;

            ++shift;
        }

        std::uint32_t bitCount = 0;

        while (
            (
                shiftedMask &
                1u
            ) !=
            0u)
        {
            ++bitCount;

            shiftedMask >>=
                1u;
        }

        if (bitCount == 0)
        {
            return defaultValue;
        }

        const std::uint32_t value =
            (
                pixel &
                mask
            ) >>
            shift;

        const std::uint64_t maximum =
            bitCount >= 32
                ? 0xFFFFFFFFull
                : (
                    1ull <<
                    bitCount
                  ) -
                    1ull;

        if (maximum == 0)
        {
            return defaultValue;
        }

        return
            static_cast<std::uint8_t>(
                (
                    static_cast<std::uint64_t>(
                        value) *
                        255ull +
                    maximum /
                        2ull
                ) /
                maximum);
    }

    bool DecodeUncompressed(
        const std::span<const std::byte> data,
        const std::uint32_t headerFlags,
        const std::uint32_t pitchOrLinearSize,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t pixelFlags,
        const std::uint32_t bitCount,
        const std::uint32_t rMask,
        const std::uint32_t gMask,
        const std::uint32_t bMask,
        const std::uint32_t aMask,
        core::images::RgbaImage& output,
        std::string& error)
    {
        if (bitCount == 0 ||
            bitCount >
                32)
        {
            error =
                "Unsupported uncompressed DDS bit depth: " +
                std::to_string(
                    bitCount);

            return false;
        }

        const std::size_t bytesPerPixel =
            (
                bitCount +
                7u
            ) /
            8u;

        const std::size_t minimumPitch =
            static_cast<std::size_t>(
                width) *
            bytesPerPixel;

        std::size_t rowPitch =
            minimumPitch;

        if (
            (
                headerFlags &
                DdsdPitch
            ) !=
                0u &&
            pitchOrLinearSize >=
                minimumPitch)
        {
            rowPitch =
                pitchOrLinearSize;
        }

        const std::size_t requiredSize =
            rowPitch *
            static_cast<std::size_t>(
                height);

        if (data.size() <
            requiredSize)
        {
            error =
                "Uncompressed DDS pixel data is truncated.";

            return false;
        }

        CreateImage(
            width,
            height,
            output);

        for (std::uint32_t y = 0;
             y < height;
             ++y)
        {
            const std::size_t rowOffset =
                static_cast<std::size_t>(
                    y) *
                rowPitch;

            for (std::uint32_t x = 0;
                 x < width;
                 ++x)
            {
                const std::size_t pixelOffset =
                    rowOffset +
                    static_cast<std::size_t>(
                        x) *
                        bytesPerPixel;

                std::uint32_t pixel = 0;

                for (std::size_t byteIndex = 0;
                     byteIndex <
                        bytesPerPixel;
                     ++byteIndex)
                {
                    pixel |=
                        static_cast<std::uint32_t>(
                            std::to_integer<std::uint8_t>(
                                data[
                                    pixelOffset +
                                    byteIndex]))
                        <<
                        (
                            byteIndex *
                            8u
                        );
                }

                Colour colour;

                if (
                    (
                        pixelFlags &
                        DdpfRgb
                    ) !=
                    0u)
                {
                    colour.r =
                        DecodeMaskedChannel(
                            pixel,
                            rMask,
                            0);

                    colour.g =
                        DecodeMaskedChannel(
                            pixel,
                            gMask,
                            0);

                    colour.b =
                        DecodeMaskedChannel(
                            pixel,
                            bMask,
                            0);

                    colour.a =
                        DecodeMaskedChannel(
                            pixel,
                            aMask,
                            255);
                }
                else if (
                    (
                        pixelFlags &
                        DdpfLuminance
                    ) !=
                    0u)
                {
                    const std::uint8_t luminance =
                        DecodeMaskedChannel(
                            pixel,
                            rMask,
                            static_cast<std::uint8_t>(
                                pixel &
                                0xFFu));

                    colour.r =
                        luminance;

                    colour.g =
                        luminance;

                    colour.b =
                        luminance;

                    colour.a =
                        DecodeMaskedChannel(
                            pixel,
                            aMask,
                            255);
                }
                else if (
                    (
                        pixelFlags &
                        DdpfAlpha
                    ) !=
                    0u)
                {
                    colour.r =
                        255;

                    colour.g =
                        255;

                    colour.b =
                        255;

                    colour.a =
                        DecodeMaskedChannel(
                            pixel,
                            aMask,
                            static_cast<std::uint8_t>(
                                pixel &
                                0xFFu));
                }
                else
                {
                    error =
                        "Unsupported uncompressed DDS pixel format.";

                    return false;
                }

                WritePixel(
                    output,
                    x,
                    y,
                    colour);
            }
        }

        return true;
    }

    bool DecodeLegacyDds(
        const std::span<const std::byte> data,
        core::images::RgbaImage& output,
        std::string& error)
    {
        output = {};
        error.clear();

        if (data.size() <
            DataOffset)
        {
            error =
                "DDS file is too small.";

            return false;
        }

        std::uint32_t magic = 0;
        std::uint32_t headerSize = 0;

        std::uint32_t headerFlags = 0;

        std::uint32_t height = 0;
        std::uint32_t width = 0;

        std::uint32_t pitchOrLinearSize = 0;

        std::uint32_t pixelFormatSize = 0;
        std::uint32_t pixelFlags = 0;

        std::uint32_t fourCC = 0;
        std::uint32_t bitCount = 0;

        std::uint32_t rMask = 0;
        std::uint32_t gMask = 0;
        std::uint32_t bMask = 0;
        std::uint32_t aMask = 0;

        if (!ReadUInt32(
                data,
                0,
                magic) ||
            !ReadUInt32(
                data,
                4,
                headerSize) ||
            !ReadUInt32(
                data,
                8,
                headerFlags) ||
            !ReadUInt32(
                data,
                12,
                height) ||
            !ReadUInt32(
                data,
                16,
                width) ||
            !ReadUInt32(
                data,
                20,
                pitchOrLinearSize) ||
            !ReadUInt32(
                data,
                76,
                pixelFormatSize) ||
            !ReadUInt32(
                data,
                80,
                pixelFlags) ||
            !ReadUInt32(
                data,
                84,
                fourCC) ||
            !ReadUInt32(
                data,
                88,
                bitCount) ||
            !ReadUInt32(
                data,
                92,
                rMask) ||
            !ReadUInt32(
                data,
                96,
                gMask) ||
            !ReadUInt32(
                data,
                100,
                bMask) ||
            !ReadUInt32(
                data,
                104,
                aMask))
        {
            error =
                "DDS header is truncated.";

            return false;
        }

        if (magic !=
            DdsMagic)
        {
            error =
                "DDS magic is invalid.";

            return false;
        }

        if (headerSize !=
                HeaderSize ||
            pixelFormatSize !=
                PixelFormatSize)
        {
            error =
                "DDS header size is invalid.";

            return false;
        }

        if (width == 0 ||
            height == 0)
        {
            error =
                "DDS dimensions are invalid.";

            return false;
        }

        const std::span<const std::byte>
            payload =
                data.subspan(
                    DataOffset);

        if (
            (
                pixelFlags &
                DdpfFourCC
            ) !=
            0u)
        {
            if (fourCC ==
                FourCcDxt1)
            {
                return DecodeBc1(
                    payload,
                    width,
                    height,
                    output,
                    error);
            }

            if (fourCC ==
                FourCcDxt3)
            {
                return DecodeBc2(
                    payload,
                    width,
                    height,
                    output,
                    error);
            }

            if (fourCC ==
                FourCcDxt5)
            {
                return DecodeBc3(
                    payload,
                    width,
                    height,
                    output,
                    error);
            }

            error =
                "Unsupported DDS FourCC: " +
                std::to_string(
                    fourCC);

            return false;
        }

        return DecodeUncompressed(
            payload,
            headerFlags,
            pitchOrLinearSize,
            width,
            height,
            pixelFlags,
            bitCount,
            rMask,
            gMask,
            bMask,
            aMask,
            output,
            error);
    }
}

namespace core::images
{
    bool DdsDecoder::Decode(
        const std::span<const std::byte> data,
        RgbaImage& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        WicImageDecoder wicDecoder;

        std::string wicError;

        if (wicDecoder.Decode(
                data,
                output,
                wicError))
        {
            return true;
        }

        std::string nativeError;

        if (DecodeLegacyDds(
                data,
                output,
                nativeError))
        {
            return true;
        }

        error =
            "WIC failed: " +
            wicError +
            "; native DDS fallback failed: " +
            nativeError;

        return false;
    }
}