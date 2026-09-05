#include "Core/Compression/ZlibDecoder.h"

#include "Core/Compression/DeflateDecoder.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace
{
    constexpr std::uint32_t AdlerModulo =
        65521u;

    std::uint32_t ComputeAdler32(
        const std::span<const std::byte> data) noexcept
    {
        std::uint32_t a =
            1u;

        std::uint32_t b =
            0u;

        for (const std::byte value :
             data)
        {
            a =
                (
                    a +
                    std::to_integer<std::uint8_t>(
                        value)
                ) %
                AdlerModulo;

            b =
                (
                    b +
                    a
                ) %
                AdlerModulo;
        }

        return
            (
                b <<
                16u
            ) |
            a;
    }

    std::uint32_t ReadBigEndianUInt32(
        const std::byte* data) noexcept
    {
        return
            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[0]))
                << 24u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[1]))
                << 16u
            ) |

            (
                static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(
                        data[2]))
                << 8u
            ) |

            static_cast<std::uint32_t>(
                std::to_integer<std::uint8_t>(
                    data[3]));
    }
}

namespace core::compression
{
    bool ZlibDecoder::Decode(
        const std::span<const std::byte> compressed,
        const std::size_t expectedSize,
        std::vector<std::byte>& output,
        std::string& error) const
    {
        output.clear();
        error.clear();

        if (compressed.size() <
            6)
        {
            error =
                "ZLIB stream is too small.";

            return false;
        }

        const std::uint8_t cmf =
            std::to_integer<std::uint8_t>(
                compressed[0]);

        const std::uint8_t flg =
            std::to_integer<std::uint8_t>(
                compressed[1]);

        if ((cmf & 0x0Fu) !=
            8u)
        {
            error =
                "ZLIB stream does not use DEFLATE.";

            return false;
        }

        const std::uint32_t header =
            (
                static_cast<std::uint32_t>(
                    cmf) <<
                8u
            ) |
            flg;

        if ((header % 31u) !=
            0u)
        {
            error =
                "ZLIB header checksum is invalid.";

            return false;
        }

        if ((flg & 0x20u) !=
            0u)
        {
            error =
                "ZLIB preset dictionaries are not supported.";

            return false;
        }

        const std::size_t deflateSize =
            compressed.size() -
            6;

        const std::span<const std::byte>
            deflateData =
                compressed.subspan(
                    2,
                    deflateSize);

        DeflateDecoder decoder;

        if (!decoder.Decode(
                deflateData,
                expectedSize,
                output,
                error))
        {
            error =
                "ZLIB DEFLATE decode failed: " +
                error;

            return false;
        }

        const std::uint32_t expectedAdler =
            ReadBigEndianUInt32(
                compressed.data() +
                compressed.size() -
                4);

        const std::uint32_t actualAdler =
            ComputeAdler32(
                output);

        if (expectedAdler !=
            actualAdler)
        {
            output.clear();

            error =
                "ZLIB Adler32 checksum mismatch.";

            return false;
        }

        return true;
    }
}