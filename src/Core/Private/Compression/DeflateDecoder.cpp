#include "Core/Compression/DeflateDecoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace
{
    class BitReader final
    {
    public:
        explicit BitReader(
            const std::span<const std::byte> data) noexcept
            : data_(data)
        {
        }

        [[nodiscard]]
        bool ReadBit(
            std::uint32_t& output) noexcept
        {
            if (byteOffset_ >=
                data_.size())
            {
                return false;
            }

            const std::uint8_t value =
                std::to_integer<std::uint8_t>(
                    data_[byteOffset_]);

            output =
                (value >> bitOffset_) &
                1u;

            ++bitOffset_;

            if (bitOffset_ == 8)
            {
                bitOffset_ = 0;
                ++byteOffset_;
            }

            return true;
        }

        [[nodiscard]]
        bool ReadBits(
            const std::uint32_t count,
            std::uint32_t& output) noexcept
        {
            output = 0;

            for (std::uint32_t bitIndex = 0;
                 bitIndex < count;
                 ++bitIndex)
            {
                std::uint32_t bit = 0;

                if (!ReadBit(bit))
                {
                    return false;
                }

                output |=
                    bit <<
                    bitIndex;
            }

            return true;
        }

        void AlignToByte() noexcept
        {
            if (bitOffset_ == 0)
            {
                return;
            }

            bitOffset_ = 0;
            ++byteOffset_;
        }

        [[nodiscard]]
        bool ReadAlignedUInt16(
            std::uint16_t& output) noexcept
        {
            if (bitOffset_ != 0)
            {
                return false;
            }

            if (byteOffset_ >
                    data_.size() ||
                2 >
                    data_.size() -
                        byteOffset_)
            {
                return false;
            }

            const std::uint16_t low =
                std::to_integer<std::uint8_t>(
                    data_[byteOffset_ + 0]);

            const std::uint16_t high =
                std::to_integer<std::uint8_t>(
                    data_[byteOffset_ + 1]);

            output =
                low |
                static_cast<std::uint16_t>(
                    high << 8u);

            byteOffset_ += 2;

            return true;
        }

        [[nodiscard]]
        bool CopyAligned(
            const std::size_t size,
            std::vector<std::byte>& output) noexcept
        {
            if (bitOffset_ != 0)
            {
                return false;
            }

            if (byteOffset_ >
                    data_.size() ||
                size >
                    data_.size() -
                        byteOffset_)
            {
                return false;
            }

            output.insert(
                output.end(),
                data_.begin() +
                    static_cast<std::ptrdiff_t>(
                        byteOffset_),
                data_.begin() +
                    static_cast<std::ptrdiff_t>(
                        byteOffset_ +
                        size));

            byteOffset_ += size;

            return true;
        }

    private:
        std::span<const std::byte> data_;

        std::size_t byteOffset_ = 0;
        std::uint32_t bitOffset_ = 0;
    };

    class HuffmanTable final
    {
    public:
        [[nodiscard]]
        bool Build(
            const std::span<const std::uint8_t> lengths,
            const bool allowEmpty,
            std::string& error)
        {
            counts_.fill(0);
            firstCodes_.fill(0);
            firstSymbols_.fill(0);

            symbols_.clear();

            maxBits_ = 0;

            std::size_t usedSymbolCount = 0;

            for (const std::uint8_t length :
                 lengths)
            {
                if (length > 15)
                {
                    error =
                        "DEFLATE Huffman code length exceeds 15 bits.";

                    return false;
                }

                if (length == 0)
                {
                    continue;
                }

                ++counts_[length];
                ++usedSymbolCount;

                if (length >
                    maxBits_)
                {
                    maxBits_ =
                        length;
                }
            }

            if (usedSymbolCount == 0)
            {
                if (allowEmpty)
                {
                    return true;
                }

                error =
                    "DEFLATE Huffman table is empty.";

                return false;
            }

            int remainingCodes = 1;

            for (std::uint32_t bitLength = 1;
                 bitLength <= 15;
                 ++bitLength)
            {
                remainingCodes =
                    (
                        remainingCodes <<
                        1
                    ) -
                    static_cast<int>(
                        counts_[bitLength]);

                if (remainingCodes < 0)
                {
                    error =
                        "DEFLATE Huffman table is oversubscribed.";

                    return false;
                }
            }

            std::uint32_t code = 0;
            std::uint16_t symbolOffset = 0;

            for (std::uint32_t bitLength = 1;
                 bitLength <= 15;
                 ++bitLength)
            {
                code =
                    (
                        code +
                        counts_[
                            bitLength - 1]
                    ) <<
                    1u;

                firstCodes_[bitLength] =
                    code;

                firstSymbols_[bitLength] =
                    symbolOffset;

                symbolOffset =
                    static_cast<std::uint16_t>(
                        symbolOffset +
                        counts_[bitLength]);
            }

            symbols_.resize(
                usedSymbolCount);

            std::array<
                std::uint16_t,
                16>
                nextSymbol =
                    firstSymbols_;

            for (std::size_t symbol = 0;
                 symbol < lengths.size();
                 ++symbol)
            {
                const std::uint8_t length =
                    lengths[symbol];

                if (length == 0)
                {
                    continue;
                }

                const std::uint16_t destination =
                    nextSymbol[length]++;

                if (destination >=
                    symbols_.size())
                {
                    error =
                        "DEFLATE Huffman symbol table is invalid.";

                    return false;
                }

                symbols_[destination] =
                    static_cast<std::uint16_t>(
                        symbol);
            }

            return true;
        }

        [[nodiscard]]
        bool Decode(
            BitReader& reader,
            std::uint32_t& output,
            std::string& error) const
        {
            if (maxBits_ == 0)
            {
                error =
                    "DEFLATE attempted to use an empty Huffman table.";

                return false;
            }

            std::uint32_t code = 0;

            for (std::uint32_t bitLength = 1;
                 bitLength <= maxBits_;
                 ++bitLength)
            {
                std::uint32_t bit = 0;

                if (!reader.ReadBit(bit))
                {
                    error =
                        "DEFLATE Huffman code is truncated.";

                    return false;
                }

                code =
                    (
                        code <<
                        1u
                    ) |
                    bit;

                const std::uint32_t firstCode =
                    firstCodes_[bitLength];

                const std::uint32_t count =
                    counts_[bitLength];

                if (count == 0)
                {
                    continue;
                }

                if (code <
                    firstCode)
                {
                    continue;
                }

                const std::uint32_t relativeCode =
                    code -
                    firstCode;

                if (relativeCode >=
                    count)
                {
                    continue;
                }

                const std::size_t symbolIndex =
                    static_cast<std::size_t>(
                        firstSymbols_[bitLength]) +
                    relativeCode;

                if (symbolIndex >=
                    symbols_.size())
                {
                    error =
                        "DEFLATE Huffman symbol index is invalid.";

                    return false;
                }

                output =
                    symbols_[symbolIndex];

                return true;
            }

            error =
                "DEFLATE contains an invalid Huffman code.";

            return false;
        }

        [[nodiscard]]
        bool Empty() const noexcept
        {
            return
                maxBits_ == 0;
        }

    private:
        std::array<
            std::uint16_t,
            16>
            counts_{};

        std::array<
            std::uint32_t,
            16>
            firstCodes_{};

        std::array<
            std::uint16_t,
            16>
            firstSymbols_{};

        std::vector<std::uint16_t>
            symbols_;

        std::uint32_t maxBits_ = 0;
    };

    constexpr std::array<
        std::uint16_t,
        29>
        LengthBase
    {
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        13,
        15,
        17,
        19,
        23,
        27,
        31,
        35,
        43,
        51,
        59,
        67,
        83,
        99,
        115,
        131,
        163,
        195,
        227,
        258
    };

    constexpr std::array<
        std::uint8_t,
        29>
        LengthExtra
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        2,
        2,
        2,
        2,
        3,
        3,
        3,
        3,
        4,
        4,
        4,
        4,
        5,
        5,
        5,
        5,
        0
    };

    constexpr std::array<
        std::uint16_t,
        30>
        DistanceBase
    {
        1,
        2,
        3,
        4,
        5,
        7,
        9,
        13,
        17,
        25,
        33,
        49,
        65,
        97,
        129,
        193,
        257,
        385,
        513,
        769,
        1025,
        1537,
        2049,
        3073,
        4097,
        6145,
        8193,
        12289,
        16385,
        24577
    };

    constexpr std::array<
        std::uint8_t,
        30>
        DistanceExtra
    {
        0,
        0,
        0,
        0,
        1,
        1,
        2,
        2,
        3,
        3,
        4,
        4,
        5,
        5,
        6,
        6,
        7,
        7,
        8,
        8,
        9,
        9,
        10,
        10,
        11,
        11,
        12,
        12,
        13,
        13
    };

    [[nodiscard]]
    bool AppendLiteral(
        const std::uint8_t value,
        const std::size_t expectedSize,
        std::vector<std::byte>& output,
        std::string& error)
    {
        if (output.size() >=
            expectedSize)
        {
            error =
                "DEFLATE output exceeds expected size.";

            return false;
        }

        output.push_back(
            static_cast<std::byte>(
                value));

        return true;
    }

    [[nodiscard]]
    bool DecodeCompressedBlock(
        BitReader& reader,
        const HuffmanTable& literalTable,
        const HuffmanTable& distanceTable,
        const std::size_t expectedSize,
        std::vector<std::byte>& output,
        std::string& error)
    {
        for (;;)
        {
            std::uint32_t symbol = 0;

            if (!literalTable.Decode(
                    reader,
                    symbol,
                    error))
            {
                return false;
            }

            if (symbol < 256)
            {
                if (!AppendLiteral(
                        static_cast<std::uint8_t>(
                            symbol),
                        expectedSize,
                        output,
                        error))
                {
                    return false;
                }

                continue;
            }

            if (symbol == 256)
            {
                return true;
            }

            if (symbol < 257 ||
                symbol > 285)
            {
                error =
                    "DEFLATE contains invalid length symbol.";

                return false;
            }

            const std::size_t lengthIndex =
                symbol -
                257;

            std::uint32_t extraLength = 0;

            if (LengthExtra[lengthIndex] != 0)
            {
                if (!reader.ReadBits(
                        LengthExtra[lengthIndex],
                        extraLength))
                {
                    error =
                        "DEFLATE length extra bits are truncated.";

                    return false;
                }
            }

            const std::size_t length =
                static_cast<std::size_t>(
                    LengthBase[lengthIndex]) +
                extraLength;

            if (distanceTable.Empty())
            {
                error =
                    "DEFLATE length references an empty distance table.";

                return false;
            }

            std::uint32_t distanceSymbol = 0;

            if (!distanceTable.Decode(
                    reader,
                    distanceSymbol,
                    error))
            {
                return false;
            }

            if (distanceSymbol >=
                DistanceBase.size())
            {
                error =
                    "DEFLATE contains invalid distance symbol.";

                return false;
            }

            std::uint32_t extraDistance = 0;

            if (DistanceExtra[
                    distanceSymbol] != 0)
            {
                if (!reader.ReadBits(
                        DistanceExtra[
                            distanceSymbol],
                        extraDistance))
                {
                    error =
                        "DEFLATE distance extra bits are truncated.";

                    return false;
                }
            }

            const std::size_t distance =
                static_cast<std::size_t>(
                    DistanceBase[
                        distanceSymbol]) +
                extraDistance;

            if (distance == 0 ||
                distance >
                    output.size())
            {
                error =
                    "DEFLATE distance exceeds decoded output.";

                return false;
            }

            if (length >
                expectedSize -
                    output.size())
            {
                error =
                    "DEFLATE output exceeds expected size.";

                return false;
            }

            for (std::size_t index = 0;
                 index < length;
                 ++index)
            {
                const std::size_t sourceIndex =
                    output.size() -
                    distance;

                output.push_back(
                    output[sourceIndex]);
            }
        }
    }

    [[nodiscard]]
    bool BuildFixedTables(
        HuffmanTable& literalTable,
        HuffmanTable& distanceTable,
        std::string& error)
    {
        std::array<
            std::uint8_t,
            288>
            literalLengths{};

        for (std::size_t symbol = 0;
             symbol <= 143;
             ++symbol)
        {
            literalLengths[symbol] =
                8;
        }

        for (std::size_t symbol = 144;
             symbol <= 255;
             ++symbol)
        {
            literalLengths[symbol] =
                9;
        }

        for (std::size_t symbol = 256;
             symbol <= 279;
             ++symbol)
        {
            literalLengths[symbol] =
                7;
        }

        for (std::size_t symbol = 280;
             symbol <= 287;
             ++symbol)
        {
            literalLengths[symbol] =
                8;
        }

        std::array<
            std::uint8_t,
            32>
            distanceLengths{};

        distanceLengths.fill(
            5);

        return
            literalTable.Build(
                literalLengths,
                false,
                error) &&
            distanceTable.Build(
                distanceLengths,
                false,
                error);
    }

    [[nodiscard]]
    bool BuildDynamicTables(
        BitReader& reader,
        HuffmanTable& literalTable,
        HuffmanTable& distanceTable,
        std::string& error)
    {
        std::uint32_t rawLiteralCount = 0;
        std::uint32_t rawDistanceCount = 0;
        std::uint32_t rawCodeLengthCount = 0;

        if (!reader.ReadBits(
                5,
                rawLiteralCount) ||
            !reader.ReadBits(
                5,
                rawDistanceCount) ||
            !reader.ReadBits(
                4,
                rawCodeLengthCount))
        {
            error =
                "DEFLATE dynamic header is truncated.";

            return false;
        }

        const std::size_t literalCount =
            rawLiteralCount +
            257u;

        const std::size_t distanceCount =
            rawDistanceCount +
            1u;

        const std::size_t codeLengthCount =
            rawCodeLengthCount +
            4u;

        if (literalCount > 286 ||
            distanceCount > 32 ||
            codeLengthCount > 19)
        {
            error =
                "DEFLATE dynamic table dimensions are invalid.";

            return false;
        }

        constexpr std::array<
            std::uint8_t,
            19>
            CodeLengthOrder
        {
            16,
            17,
            18,
            0,
            8,
            7,
            9,
            6,
            10,
            5,
            11,
            4,
            12,
            3,
            13,
            2,
            14,
            1,
            15
        };

        std::array<
            std::uint8_t,
            19>
            codeLengthLengths{};

        for (std::size_t index = 0;
             index < codeLengthCount;
             ++index)
        {
            std::uint32_t length = 0;

            if (!reader.ReadBits(
                    3,
                    length))
            {
                error =
                    "DEFLATE code-length table is truncated.";

                return false;
            }

            codeLengthLengths[
                CodeLengthOrder[index]] =
                static_cast<std::uint8_t>(
                    length);
        }

        HuffmanTable codeLengthTable;

        if (!codeLengthTable.Build(
                codeLengthLengths,
                false,
                error))
        {
            return false;
        }

        const std::size_t totalLengthCount =
            literalCount +
            distanceCount;

        std::vector<std::uint8_t>
            lengths(
                totalLengthCount,
                0);

        std::size_t lengthIndex = 0;

        while (lengthIndex <
               lengths.size())
        {
            std::uint32_t symbol = 0;

            if (!codeLengthTable.Decode(
                    reader,
                    symbol,
                    error))
            {
                return false;
            }

            if (symbol <= 15)
            {
                lengths[lengthIndex++] =
                    static_cast<std::uint8_t>(
                        symbol);

                continue;
            }

            if (symbol == 16)
            {
                if (lengthIndex == 0)
                {
                    error =
                        "DEFLATE repeat code has no previous length.";

                    return false;
                }

                std::uint32_t extra = 0;

                if (!reader.ReadBits(
                        2,
                        extra))
                {
                    error =
                        "DEFLATE repeat code is truncated.";

                    return false;
                }

                const std::size_t repeatCount =
                    extra +
                    3u;

                if (repeatCount >
                    lengths.size() -
                        lengthIndex)
                {
                    error =
                        "DEFLATE repeat exceeds code-length table.";

                    return false;
                }

                const std::uint8_t previous =
                    lengths[
                        lengthIndex -
                        1];

                for (std::size_t repeat = 0;
                     repeat < repeatCount;
                     ++repeat)
                {
                    lengths[lengthIndex++] =
                        previous;
                }

                continue;
            }

            if (symbol == 17)
            {
                std::uint32_t extra = 0;

                if (!reader.ReadBits(
                        3,
                        extra))
                {
                    error =
                        "DEFLATE zero repeat is truncated.";

                    return false;
                }

                const std::size_t repeatCount =
                    extra +
                    3u;

                if (repeatCount >
                    lengths.size() -
                        lengthIndex)
                {
                    error =
                        "DEFLATE zero repeat exceeds code-length table.";

                    return false;
                }

                lengthIndex +=
                    repeatCount;

                continue;
            }

            if (symbol == 18)
            {
                std::uint32_t extra = 0;

                if (!reader.ReadBits(
                        7,
                        extra))
                {
                    error =
                        "DEFLATE long zero repeat is truncated.";

                    return false;
                }

                const std::size_t repeatCount =
                    extra +
                    11u;

                if (repeatCount >
                    lengths.size() -
                        lengthIndex)
                {
                    error =
                        "DEFLATE long zero repeat exceeds code-length table.";

                    return false;
                }

                lengthIndex +=
                    repeatCount;

                continue;
            }

            error =
                "DEFLATE contains invalid code-length symbol.";

            return false;
        }

        const std::span<const std::uint8_t>
            literalLengths(
                lengths.data(),
                literalCount);

        const std::span<const std::uint8_t>
            distanceLengths(
                lengths.data() +
                    literalCount,
                distanceCount);

        if (literalLengths.size() <= 256 ||
            literalLengths[256] == 0)
        {
            error =
                "DEFLATE literal table has no end-of-block code.";

            return false;
        }

        if (!literalTable.Build(
                literalLengths,
                false,
                error))
        {
            return false;
        }

        if (!distanceTable.Build(
                distanceLengths,
                true,
                error))
        {
            return false;
        }

        return true;
    }
}

namespace core::compression
{
    bool DeflateDecoder::Decode(
        const std::span<const std::byte> compressed,
        const std::size_t expectedSize,
        std::vector<std::byte>& output,
        std::string& error) const
    {
        output.clear();
        error.clear();

        if (compressed.empty() &&
            expectedSize != 0)
        {
            error =
                "DEFLATE input is empty.";

            return false;
        }

        output.reserve(
            expectedSize);

        BitReader reader(
            compressed);

        bool finalBlock = false;

        while (!finalBlock)
        {
            std::uint32_t finalBit = 0;
            std::uint32_t blockType = 0;

            if (!reader.ReadBit(
                    finalBit) ||
                !reader.ReadBits(
                    2,
                    blockType))
            {
                error =
                    "DEFLATE block header is truncated.";

                return false;
            }

            finalBlock =
                finalBit != 0;

            if (blockType == 0)
            {
                reader.AlignToByte();

                std::uint16_t length = 0;
                std::uint16_t inverseLength = 0;

                if (!reader.ReadAlignedUInt16(
                        length) ||
                    !reader.ReadAlignedUInt16(
                        inverseLength))
                {
                    error =
                        "DEFLATE stored block header is truncated.";

                    return false;
                }

                if (static_cast<std::uint16_t>(
                        length ^
                        0xFFFFu) !=
                    inverseLength)
                {
                    error =
                        "DEFLATE stored block length check failed.";

                    return false;
                }

                if (length >
                    expectedSize -
                        output.size())
                {
                    error =
                        "DEFLATE stored block exceeds expected size.";

                    return false;
                }

                if (!reader.CopyAligned(
                        length,
                        output))
                {
                    error =
                        "DEFLATE stored block is truncated.";

                    return false;
                }

                continue;
            }

            if (blockType == 3)
            {
                error =
                    "DEFLATE contains reserved block type.";

                return false;
            }

            HuffmanTable literalTable;
            HuffmanTable distanceTable;

            if (blockType == 1)
            {
                if (!BuildFixedTables(
                        literalTable,
                        distanceTable,
                        error))
                {
                    return false;
                }
            }
            else
            {
                if (!BuildDynamicTables(
                        reader,
                        literalTable,
                        distanceTable,
                        error))
                {
                    return false;
                }
            }

            if (!DecodeCompressedBlock(
                    reader,
                    literalTable,
                    distanceTable,
                    expectedSize,
                    output,
                    error))
            {
                return false;
            }
        }

        if (output.size() !=
            expectedSize)
        {
            error =
                "DEFLATE output size does not match ZIP entry size.";

            output.clear();

            return false;
        }

        return true;
    }
}