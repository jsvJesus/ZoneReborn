#include "Core/Resources/ZipArchiveReader.h"

#include <cstdint>
#include <limits>
#include <string>

namespace
{
    constexpr std::uint32_t LocalHeaderSignature =
        0x04034B50u;

    constexpr std::uint32_t CentralHeaderSignature =
        0x02014B50u;

    constexpr std::uint32_t EndSignature =
        0x06054B50u;

    constexpr std::size_t EndHeaderSize =
        22;

    constexpr std::size_t MaximumZipComment =
        65535;

    bool ReadUInt16(
        const std::span<const std::byte> data,
        const std::size_t offset,
        std::uint16_t& output) noexcept
    {
        if (offset > data.size() ||
            sizeof(std::uint16_t) >
                data.size() - offset)
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

    bool FindEndOfCentralDirectory(
        const std::span<const std::byte> archive,
        std::size_t& output) noexcept
    {
        if (archive.size() <
            EndHeaderSize)
        {
            return false;
        }

        const std::size_t minimumOffset =
            archive.size() >
                EndHeaderSize +
                MaximumZipComment
                ? archive.size() -
                    EndHeaderSize -
                    MaximumZipComment
                : 0;

        std::size_t offset =
            archive.size() -
            EndHeaderSize;

        for (;;)
        {
            std::uint32_t signature = 0;

            if (ReadUInt32(
                    archive,
                    offset,
                    signature) &&
                signature ==
                    EndSignature)
            {
                output = offset;
                return true;
            }

            if (offset ==
                minimumOffset)
            {
                break;
            }

            --offset;
        }

        return false;
    }
}

namespace core::resources
{
    bool ZipArchiveReader::ExtractStored(
        const std::span<const std::byte> archive,
        const std::string_view entryName,
        std::vector<std::byte>& output,
        std::string& error) const
    {
        output.clear();
        error.clear();

        if (archive.empty())
        {
            error =
                "ZIP archive is empty.";

            return false;
        }

        if (entryName.empty())
        {
            error =
                "ZIP entry name is empty.";

            return false;
        }

        std::size_t endOffset = 0;

        if (!FindEndOfCentralDirectory(
                archive,
                endOffset))
        {
            error =
                "ZIP central directory was not found.";

            return false;
        }

        std::uint16_t entryCount = 0;

        std::uint32_t centralSize = 0;
        std::uint32_t centralOffset32 = 0;

        if (!ReadUInt16(
                archive,
                endOffset + 10,
                entryCount) ||
            !ReadUInt32(
                archive,
                endOffset + 12,
                centralSize) ||
            !ReadUInt32(
                archive,
                endOffset + 16,
                centralOffset32))
        {
            error =
                "ZIP end record is truncated.";

            return false;
        }

        const std::size_t centralOffset =
            static_cast<std::size_t>(
                centralOffset32);

        if (centralOffset >
                archive.size() ||
            centralSize >
                archive.size() -
                    centralOffset)
        {
            error =
                "ZIP central directory is invalid.";

            return false;
        }

        std::size_t cursor =
            centralOffset;

        for (std::uint16_t entryIndex = 0;
             entryIndex < entryCount;
             ++entryIndex)
        {
            constexpr std::size_t CentralHeaderSize =
                46;

            if (cursor >
                    archive.size() ||
                CentralHeaderSize >
                    archive.size() -
                        cursor)
            {
                error =
                    "ZIP central entry is truncated.";

                return false;
            }

            std::uint32_t signature = 0;

            if (!ReadUInt32(
                    archive,
                    cursor,
                    signature) ||
                signature !=
                    CentralHeaderSignature)
            {
                error =
                    "ZIP central entry signature is invalid.";

                return false;
            }

            std::uint16_t flags = 0;
            std::uint16_t compression = 0;

            std::uint32_t compressedSize = 0;
            std::uint32_t uncompressedSize = 0;

            std::uint16_t nameLength = 0;
            std::uint16_t extraLength = 0;
            std::uint16_t commentLength = 0;

            std::uint32_t localOffset32 = 0;

            if (!ReadUInt16(
                    archive,
                    cursor + 8,
                    flags) ||
                !ReadUInt16(
                    archive,
                    cursor + 10,
                    compression) ||
                !ReadUInt32(
                    archive,
                    cursor + 20,
                    compressedSize) ||
                !ReadUInt32(
                    archive,
                    cursor + 24,
                    uncompressedSize) ||
                !ReadUInt16(
                    archive,
                    cursor + 28,
                    nameLength) ||
                !ReadUInt16(
                    archive,
                    cursor + 30,
                    extraLength) ||
                !ReadUInt16(
                    archive,
                    cursor + 32,
                    commentLength) ||
                !ReadUInt32(
                    archive,
                    cursor + 42,
                    localOffset32))
            {
                error =
                    "ZIP central entry is truncated.";

                return false;
            }

            const std::size_t nameOffset =
                cursor +
                CentralHeaderSize;

            if (nameOffset >
                    archive.size() ||
                nameLength >
                    archive.size() -
                        nameOffset)
            {
                error =
                    "ZIP entry name is truncated.";

                return false;
            }

            const std::string_view currentName(
                reinterpret_cast<const char*>(
                    archive.data() +
                    nameOffset),
                nameLength);

            if (currentName ==
                entryName)
            {
                if ((flags & 0x0001u) != 0)
                {
                    error =
                        "Encrypted ZIP entries are not supported.";

                    return false;
                }

                if (compression != 0)
                {
                    error =
                        "Terrain ZIP entry is compressed with unsupported method: " +
                        std::to_string(
                            compression);

                    return false;
                }

                if (compressedSize !=
                    uncompressedSize)
                {
                    error =
                        "Stored ZIP entry has mismatched sizes.";

                    return false;
                }

                const std::size_t localOffset =
                    static_cast<std::size_t>(
                        localOffset32);

                constexpr std::size_t LocalHeaderSize =
                    30;

                if (localOffset >
                        archive.size() ||
                    LocalHeaderSize >
                        archive.size() -
                            localOffset)
                {
                    error =
                        "ZIP local entry is truncated.";

                    return false;
                }

                std::uint32_t localSignature = 0;

                std::uint16_t localNameLength = 0;
                std::uint16_t localExtraLength = 0;

                if (!ReadUInt32(
                        archive,
                        localOffset,
                        localSignature) ||
                    localSignature !=
                        LocalHeaderSignature ||
                    !ReadUInt16(
                        archive,
                        localOffset + 26,
                        localNameLength) ||
                    !ReadUInt16(
                        archive,
                        localOffset + 28,
                        localExtraLength))
                {
                    error =
                        "ZIP local entry header is invalid.";

                    return false;
                }

                const std::size_t dataOffset =
                    localOffset +
                    LocalHeaderSize +
                    localNameLength +
                    localExtraLength;

                if (dataOffset >
                        archive.size() ||
                    compressedSize >
                        archive.size() -
                            dataOffset)
                {
                    error =
                        "ZIP entry data exceeds archive size.";

                    return false;
                }

                output.assign(
                    archive.begin() +
                        static_cast<std::ptrdiff_t>(
                            dataOffset),
                    archive.begin() +
                        static_cast<std::ptrdiff_t>(
                            dataOffset +
                            compressedSize));

                return true;
            }

            const std::size_t entrySize =
                CentralHeaderSize +
                nameLength +
                extraLength +
                commentLength;

            if (entrySize >
                archive.size() -
                    cursor)
            {
                error =
                    "ZIP central entry exceeds archive size.";

                return false;
            }

            cursor +=
                entrySize;
        }

        error =
            "ZIP entry was not found: " +
            std::string(entryName);

        return false;
    }
}