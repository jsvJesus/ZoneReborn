#include "WorldEditor/Terrain/TerrainCorrection.h"

#include "WorldEditor/Biome/BiomeStorage.h"

#include <Windows.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace world_editor::terrain {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::uint32_t magic = 0x5341425Au;
        constexpr std::uint32_t version = 1;
        constexpr std::uintmax_t maximumFileSize = 16u * 1024u * 1024u;

        void write_u32(
            std::vector<std::uint8_t>& bytes,
            const std::uint32_t value
        ) {
            for (int shift = 0; shift < 32; shift += 8) {
                bytes.push_back(static_cast<std::uint8_t>(value >> shift));
            }
        }

        void write_u64(
            std::vector<std::uint8_t>& bytes,
            const std::uint64_t value
        ) {
            write_u32(bytes, static_cast<std::uint32_t>(value));
            write_u32(bytes, static_cast<std::uint32_t>(value >> 32));
        }

        [[nodiscard]] std::uint32_t read_u32(
            const std::vector<std::uint8_t>& bytes,
            std::size_t& offset
        ) {
            if (offset > bytes.size() || bytes.size() - offset < 4) {
                throw std::runtime_error("Incomplete Terrain correction base");
            }

            std::uint32_t value = 0;

            for (int shift = 0; shift < 32; shift += 8) {
                value |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
            }

            return value;
        }

        [[nodiscard]] std::uint64_t read_u64(
            const std::vector<std::uint8_t>& bytes,
            std::size_t& offset
        ) {
            const std::uint64_t low = read_u32(bytes, offset);
            const std::uint64_t high = read_u32(bytes, offset);
            return low | (high << 32);
        }

        [[nodiscard]] std::uint32_t checksum(
            const std::vector<std::uint8_t>& bytes,
            const std::size_t count
        ) noexcept {
            std::uint32_t hash = 2166136261u;

            for (std::size_t i = 0; i < count; ++i) {
                hash = (hash ^ bytes[i]) * 16777619u;
            }

            return hash;
        }

        [[nodiscard]] bool same_grid(
            const Heightfield& a,
            const Heightfield& b
        ) noexcept {
            return a.valid() && b.valid() &&
                a.samplesX == b.samplesX &&
                a.samplesZ == b.samplesZ &&
                a.originX == b.originX &&
                a.originZ == b.originZ &&
                a.spacingMeters == b.spacingMeters;
        }

    }

    void save_correction_base(
        const Heightfield& base,
        const Heightfield& corrected,
        const fs::path& path
    ) {
        if (!same_grid(base, corrected) || path.empty()) {
            throw std::invalid_argument("Invalid Terrain correction base");
        }

        std::vector<std::uint8_t> bytes;
        bytes.reserve(44u + base.heights.size() * 4u);
        write_u32(bytes, magic);
        write_u32(bytes, version);
        write_u64(bytes, biome::terrain_source_fingerprint(corrected));
        write_u32(bytes, base.samplesX);
        write_u32(bytes, base.samplesZ);
        write_u32(bytes, std::bit_cast<std::uint32_t>(base.originX));
        write_u32(bytes, std::bit_cast<std::uint32_t>(base.originZ));
        write_u32(bytes, std::bit_cast<std::uint32_t>(base.spacingMeters));

        for (const float height : base.heights) {
            if (!std::isfinite(height)) {
                throw std::invalid_argument("Invalid Terrain correction height");
            }

            write_u32(bytes, std::bit_cast<std::uint32_t>(height));
        }

        if (bytes.size() + 4u > maximumFileSize) {
            throw std::invalid_argument("Terrain correction base is too large");
        }

        write_u32(bytes, checksum(bytes, bytes.size()));

        const auto parent = path.parent_path();

        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        fs::path pending = path;
        pending += L".pending";

        if (fs::exists(pending)) {
            throw std::runtime_error("Pending Terrain correction save exists");
        }

        try {
            {
                std::ofstream file(pending, std::ios::binary | std::ios::trunc);

                if (!file) {
                    throw std::runtime_error("Cannot create Terrain correction file");
                }

                file.write(
                    reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size())
                );
                file.flush();

                if (!file) {
                    throw std::runtime_error("Cannot finish Terrain correction save");
                }
            }

            if (!MoveFileExW(
                pending.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
                throw std::runtime_error("Cannot replace Terrain correction file");
            }
        } catch (...) {
            std::error_code error;
            fs::remove(pending, error);
            throw;
        }
    }

    Heightfield load_correction_base(
        const Heightfield& corrected,
        const fs::path& path
    ) {
        if (!corrected.valid() || !fs::exists(path) ||
            fs::file_size(path) < 44u ||
            fs::file_size(path) > maximumFileSize) {
            throw std::runtime_error("Terrain correction file is missing or invalid");
        }

        std::ifstream file(path, std::ios::binary);

        if (!file) {
            throw std::runtime_error("Cannot open Terrain correction file");
        }

        const std::vector<std::uint8_t> bytes(
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        );
        std::size_t offset = bytes.size() - 4u;
        const auto savedChecksum = read_u32(bytes, offset);

        if (checksum(bytes, bytes.size() - 4u) != savedChecksum) {
            throw std::runtime_error("Terrain correction checksum mismatch");
        }

        offset = 0;

        if (read_u32(bytes, offset) != magic ||
            read_u32(bytes, offset) != version ||
            read_u64(bytes, offset) !=
                biome::terrain_source_fingerprint(corrected)) {
            throw std::runtime_error("Terrain correction source mismatch");
        }

        Heightfield base = corrected;

        if (read_u32(bytes, offset) != base.samplesX ||
            read_u32(bytes, offset) != base.samplesZ ||
            std::bit_cast<float>(read_u32(bytes, offset)) != base.originX ||
            std::bit_cast<float>(read_u32(bytes, offset)) != base.originZ ||
            std::bit_cast<float>(read_u32(bytes, offset)) != base.spacingMeters ||
            bytes.size() - offset != base.heights.size() * 4u + 4u) {
            throw std::runtime_error("Terrain correction grid mismatch");
        }

        for (float& height : base.heights) {
            height = std::bit_cast<float>(read_u32(bytes, offset));

            if (!std::isfinite(height)) {
                throw std::runtime_error("Invalid Terrain correction height");
            }
        }

        return base;
    }

}
