#include "Studio/BuildLightsLoader.h"

#include <bit>
#include <cstddef>
#include <fstream>
#include <utility>

namespace studio
{
    namespace
    {
        constexpr std::size_t RecordSize = 104;
        constexpr std::size_t MaximumFileSize =
            64u * 1024u * 1024u;

        constexpr std::uint32_t CompressedChunkFlag =
            0x80000000u;

        static_assert(sizeof(float) == 4);
        static_assert(sizeof(std::uint32_t) == 4);

        std::uint16_t Read16(
            const std::uint8_t* bytes) noexcept
        {
            return
                static_cast<std::uint16_t>(bytes[0]) |
                (static_cast<std::uint16_t>(bytes[1]) << 8u);
        }

        std::uint32_t Read32(
            const std::uint8_t* bytes) noexcept
        {
            return
                static_cast<std::uint32_t>(bytes[0]) |
                (static_cast<std::uint32_t>(bytes[1]) << 8u) |
                (static_cast<std::uint32_t>(bytes[2]) << 16u) |
                (static_cast<std::uint32_t>(bytes[3]) << 24u);
        }

        float ReadFloat(
            const std::uint8_t* bytes) noexcept
        {
            return std::bit_cast<float>(Read32(bytes));
        }

        BuildLightRecord ReadRecord(
            const std::uint8_t* bytes) noexcept
        {
            BuildLightRecord record;

            record.type = Read16(bytes);
            record.level = Read16(bytes + 2);

            std::size_t offset = 4;

            auto readFloat = [&]() noexcept
            {
                const float value =
                    ReadFloat(bytes + offset);

                offset += 4;

                return value;
            };

            for (float& value : record.diffuse)
                value = readFloat();

            for (float& value : record.position)
                value = readFloat();

            for (float& value : record.direction)
                value = readFloat();

            record.range = readFloat();
            record.rangeSquared = readFloat();

            record.attenuation0 = readFloat();
            record.attenuation1 = readFloat();
            record.attenuation2 = readFloat();

            record.energy = readFloat();

            for (float& value : record.triangle)
                value = readFloat();

            record.unknown =
                Read32(bytes + offset);

            return record;
        }

        bool ReadChunkRecords(
            const std::uint8_t* bytes,
            std::size_t size,
            std::vector<BuildLightRecord>& records)
        {
            if (size % RecordSize != 0)
                return false;

            records.reserve(size / RecordSize);

            for (std::size_t offset = 0;
                 offset < size;
                 offset += RecordSize)
            {
                records.push_back(
                    ReadRecord(bytes + offset));
            }

            return true;
        }
    }

    bool LoadBuildLights(
        const std::filesystem::path& path,
        BuildLightsData& output,
        std::string& error)
    {
        error.clear();

        std::ifstream file(
            path,
            std::ios::binary | std::ios::ate);

        if (!file)
        {
            error = "Cannot open build.lights.";
            return false;
        }

        const std::streampos length =
            file.tellg();

        if (length <= 0 ||
            length >
                static_cast<std::streampos>(
                    MaximumFileSize))
        {
            error = "Invalid build.lights file size.";
            return false;
        }

        const std::size_t fileSize =
            static_cast<std::size_t>(length);

        std::vector<std::uint8_t> bytes(fileSize);

        file.seekg(0, std::ios::beg);

        if (!file.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(fileSize)))
        {
            error = "Cannot read build.lights.";
            return false;
        }

        BuildLightsData parsed;

        bool versionFound = false;

        std::array<bool, 3> found{};

        std::size_t offset = 0;

        while (offset < bytes.size())
        {
            if (bytes.size() - offset < 8)
            {
                error =
                    "Truncated build.lights chunk header.";

                return false;
            }

            const std::uint32_t rawId =
                Read32(bytes.data() + offset);

            const std::uint32_t chunkSize =
                Read32(bytes.data() + offset + 4);

            offset += 8;

            if (chunkSize > bytes.size() - offset)
            {
                error =
                    "build.lights chunk exceeds file boundaries.";

                return false;
            }

            const std::uint32_t id =
                rawId & ~CompressedChunkFlag;

            const auto* chunk =
                bytes.data() + offset;

            if (id == 10)
            {
                if (versionFound ||
                    rawId != id ||
                    chunkSize != 2)
                {
                    error =
                        "Invalid build.lights version chunk.";

                    return false;
                }

                parsed.version = Read16(chunk);

                versionFound = true;
            }
            else if (id <= 2)
            {
                if (found[id] || rawId != id)
                {
                    error =
                        "Duplicate or compressed build.lights light chunk.";

                    return false;
                }

                found[id] = true;

                auto* destination =
                    id == 0 ? &parsed.rgb :
                    id == 1 ? &parsed.hemi :
                              &parsed.sun;

                if (!ReadChunkRecords(
                    chunk,
                    chunkSize,
                    *destination))
                {
                    error =
                        "Unsupported build.lights record size.";

                    return false;
                }
            }
            else
            {
                ++parsed.auxiliaryChunks;
            }

            offset += chunkSize;
        }

        if (!versionFound ||
            parsed.version != 4 ||
            (!found[0] && !found[1] && !found[2]))
        {
            error =
                "Unsupported build.lights version or missing light data.";

            return false;
        }

        output = std::move(parsed);

        return true;
    }
}