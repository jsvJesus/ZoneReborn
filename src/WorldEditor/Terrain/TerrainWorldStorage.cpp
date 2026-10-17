
#include "WorldEditor/Terrain/TerrainWorldStorage.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace world_editor::terrain {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::uint32_t formatVersion = 1;
        constexpr std::uint32_t chunkCells = 64;
        constexpr std::uint32_t maxSamples = 2049;

        constexpr std::array<char, 4> worldMagic = {'Z', 'R', 'T', 'W'};
        constexpr std::array<char, 4> chunkMagic = {'Z', 'R', 'C', 'H'};

        static_assert(sizeof(float) == sizeof(std::uint32_t));
        static_assert(std::numeric_limits<float>::is_iec559);

        void write_bytes(std::ostream& stream, const char* data, std::streamsize size) {
            stream.write(data, size);

            if (!stream) {
                throw std::runtime_error("Unable to write terrain data");
            }
        }

        void read_bytes(std::istream& stream, char* data, std::streamsize size) {
            stream.read(data, size);

            if (!stream) {
                throw std::runtime_error("Terrain file is incomplete");
            }
        }

        void write_u32(std::ostream& stream, std::uint32_t value) {
            const std::array<char, 4> bytes = {
                static_cast<char>(value & 0xFFu),
                static_cast<char>((value >> 8) & 0xFFu),
                static_cast<char>((value >> 16) & 0xFFu),
                static_cast<char>((value >> 24) & 0xFFu)
            };

            write_bytes(
                stream,
                bytes.data(),
                static_cast<std::streamsize>(bytes.size())
            );
        }

        std::uint32_t read_u32(std::istream& stream) {
            std::array<unsigned char, 4> bytes{};

            read_bytes(
                stream,
                reinterpret_cast<char*>(bytes.data()),
                4
            );

            return static_cast<std::uint32_t>(bytes[0]) |
                (static_cast<std::uint32_t>(bytes[1]) << 8) |
                (static_cast<std::uint32_t>(bytes[2]) << 16) |
                (static_cast<std::uint32_t>(bytes[3]) << 24);
        }

        void write_f32(std::ostream& stream, float value) {
            write_u32(stream, std::bit_cast<std::uint32_t>(value));
        }

        float read_f32(std::istream& stream) {
            return std::bit_cast<float>(read_u32(stream));
        }

        void check_magic(
            std::istream& stream,
            const std::array<char, 4>& expected
        ) {
            std::array<char, 4> actual{};
            read_bytes(stream, actual.data(), 4);

            if (actual != expected) {
                throw std::runtime_error("Unknown terrain file signature");
            }
        }

        void check_end(std::istream& stream) {
            if (stream.peek() != std::char_traits<char>::eof()) {
                throw std::runtime_error("Unexpected trailing data in terrain file");
            }
        }

        void validate_settings(const Settings& settings) {
            if (settings.samplesX < 2 || settings.samplesX > maxSamples ||
                settings.samplesZ < 2 || settings.samplesZ > maxSamples ||
                !std::isfinite(settings.originX) ||
                !std::isfinite(settings.originZ) ||
                !std::isfinite(settings.spacingMeters) ||
                settings.spacingMeters <= 0.0f ||
                !std::isfinite(settings.baseHeightMeters) ||
                !std::isfinite(settings.reliefMeters) ||
                settings.reliefMeters < 0.0f ||
                !std::isfinite(settings.macroScaleMeters) ||
                !std::isfinite(settings.detailScaleMeters) ||
                settings.macroScaleMeters < settings.spacingMeters * 2.0f ||
                settings.detailScaleMeters < settings.spacingMeters * 2.0f) {
                throw std::invalid_argument("Invalid world settings");
            }
        }

        void validate_world(
            const Heightfield& terrain,
            const Settings& settings
        ) {
            validate_settings(settings);

            if (!terrain.valid() ||
                terrain.samplesX != settings.samplesX ||
                terrain.samplesZ != settings.samplesZ ||
                terrain.originX != settings.originX ||
                terrain.originZ != settings.originZ ||
                terrain.spacingMeters != settings.spacingMeters) {
                throw std::invalid_argument(
                    "World settings do not match terrain data"
                );
            }

            for (const float height : terrain.heights) {
                if (!std::isfinite(height)) {
                    throw std::invalid_argument(
                        "Terrain contains non-finite height"
                    );
                }
            }
        }

        void write_manifest(
            const fs::path& path,
            const Settings& settings
        ) {
            std::ofstream output(
                path,
                std::ios::binary | std::ios::trunc
            );

            if (!output) {
                throw std::runtime_error("Unable to create world metadata");
            }

            write_bytes(output, worldMagic.data(), 4);
            write_u32(output, formatVersion);
            write_u32(output, chunkCells);

            write_u32(output, settings.samplesX);
            write_u32(output, settings.samplesZ);
            write_u32(output, settings.seed);

            write_f32(output, settings.originX);
            write_f32(output, settings.originZ);
            write_f32(output, settings.spacingMeters);
            write_f32(output, settings.baseHeightMeters);
            write_f32(output, settings.reliefMeters);
            write_f32(output, settings.macroScaleMeters);
            write_f32(output, settings.detailScaleMeters);

            output.flush();

            if (!output) {
                throw std::runtime_error(
                    "Unable to finish writing world metadata"
                );
            }
        }

        Settings read_manifest(const fs::path& path) {
            std::ifstream input(path, std::ios::binary);

            if (!input) {
                throw std::runtime_error("World metadata not found");
            }

            check_magic(input, worldMagic);

            if (read_u32(input) != formatVersion ||
                read_u32(input) != chunkCells) {
                throw std::runtime_error(
                    "Unsupported terrain world version"
                );
            }

            Settings settings;

            settings.samplesX = read_u32(input);
            settings.samplesZ = read_u32(input);
            settings.seed = read_u32(input);

            settings.originX = read_f32(input);
            settings.originZ = read_f32(input);
            settings.spacingMeters = read_f32(input);
            settings.baseHeightMeters = read_f32(input);
            settings.reliefMeters = read_f32(input);
            settings.macroScaleMeters = read_f32(input);
            settings.detailScaleMeters = read_f32(input);

            check_end(input);
            validate_settings(settings);

            return settings;
        }

        fs::path chunk_path(
            const fs::path& directory,
            std::uint32_t x,
            std::uint32_t z
        ) {
            return directory / "chunks" /
                (
                    std::to_string(x / chunkCells) + "_" +
                    std::to_string(z / chunkCells) + ".zch"
                );
        }

        std::uint32_t checksum(
            const std::vector<std::uint8_t>& data
        ) noexcept {
            std::uint32_t hash = 2166136261u;

            for (const auto byte : data) {
                hash = (hash ^ byte) * 16777619u;
            }

            return hash;
        }

        void append_float(
            std::vector<std::uint8_t>& output,
            float value
        ) {
            const std::uint32_t bits =
                std::bit_cast<std::uint32_t>(value);

            for (std::uint32_t byte = 0; byte < 4; ++byte) {
                output.push_back(
                    static_cast<std::uint8_t>(
                        (bits >> (byte * 8)) & 0xFFu
                    )
                );
            }
        }

        float unpack_float(
            const std::vector<std::uint8_t>& data,
            std::size_t offset
        ) {
            std::uint32_t bits = 0;

            for (std::uint32_t byte = 0; byte < 4; ++byte) {
                bits |= static_cast<std::uint32_t>(
                    data[offset + byte]
                ) << (byte * 8);
            }

            return std::bit_cast<float>(bits);
        }

        void write_chunk(
            const fs::path& directory,
            const Heightfield& terrain,
            std::uint32_t startX,
            std::uint32_t startZ
        ) {
            const std::uint32_t columns = (std::min)(
                chunkCells,
                terrain.samplesX - 1 - startX
            ) + 1;

            const std::uint32_t rows = (std::min)(
                chunkCells,
                terrain.samplesZ - 1 - startZ
            ) + 1;

            std::vector<std::uint8_t> payload;

            payload.reserve(
                static_cast<std::size_t>(columns) *
                rows * sizeof(float)
            );

            for (std::uint32_t z = 0; z < rows; ++z) {
                for (std::uint32_t x = 0; x < columns; ++x) {
                    const auto index =
                        static_cast<std::size_t>(startZ + z) *
                        terrain.samplesX + startX + x;

                    append_float(payload, terrain.heights[index]);
                }
            }

            std::ofstream output(
                chunk_path(directory, startX, startZ),
                std::ios::binary | std::ios::trunc
            );

            if (!output) {
                throw std::runtime_error(
                    "Unable to create terrain chunk"
                );
            }

            write_bytes(output, chunkMagic.data(), 4);
            write_u32(output, formatVersion);
            write_u32(output, startX);
            write_u32(output, startZ);
            write_u32(output, columns);
            write_u32(output, rows);
            write_u32(output, checksum(payload));

            write_bytes(
                output,
                reinterpret_cast<const char*>(payload.data()),
                static_cast<std::streamsize>(payload.size())
            );

            output.flush();

            if (!output) {
                throw std::runtime_error(
                    "Unable to finish writing terrain chunk"
                );
            }
        }

        void read_chunk(
            const fs::path& directory,
            Heightfield& terrain,
            std::vector<std::uint8_t>& loaded,
            std::uint32_t startX,
            std::uint32_t startZ
        ) {
            std::ifstream input(
                chunk_path(directory, startX, startZ),
                std::ios::binary
            );

            if (!input) {
                throw std::runtime_error("Missing terrain chunk file");
            }

            check_magic(input, chunkMagic);

            if (read_u32(input) != formatVersion) {
                throw std::runtime_error(
                    "Unsupported terrain chunk version"
                );
            }

            const auto storedX = read_u32(input);
            const auto storedZ = read_u32(input);
            const auto storedColumns = read_u32(input);
            const auto storedRows = read_u32(input);
            const auto storedChecksum = read_u32(input);

            const auto expectedColumns = (std::min)(
                chunkCells,
                terrain.samplesX - 1 - startX
            ) + 1;

            const auto expectedRows = (std::min)(
                chunkCells,
                terrain.samplesZ - 1 - startZ
            ) + 1;

            if (storedX != startX ||
                storedZ != startZ ||
                storedColumns != expectedColumns ||
                storedRows != expectedRows) {
                throw std::runtime_error(
                    "Terrain chunk dimensions do not match world"
                );
            }

            std::vector<std::uint8_t> payload(
                static_cast<std::size_t>(expectedColumns) *
                expectedRows * sizeof(float)
            );

            read_bytes(
                input,
                reinterpret_cast<char*>(payload.data()),
                static_cast<std::streamsize>(payload.size())
            );

            check_end(input);

            if (checksum(payload) != storedChecksum) {
                throw std::runtime_error(
                    "Terrain chunk checksum mismatch"
                );
            }

            for (std::uint32_t z = 0; z < expectedRows; ++z) {
                for (std::uint32_t x = 0; x < expectedColumns; ++x) {
                    const auto sampleIndex =
                        static_cast<std::size_t>(z) *
                        expectedColumns + x;

                    const float height = unpack_float(
                        payload,
                        sampleIndex * sizeof(float)
                    );

                    if (!std::isfinite(height)) {
                        throw std::runtime_error(
                            "Terrain chunk contains non-finite height"
                        );
                    }

                    const auto worldIndex =
                        static_cast<std::size_t>(startZ + z) *
                        terrain.samplesX + startX + x;

                    if (loaded[worldIndex]) {
                        if (
                            std::bit_cast<std::uint32_t>(
                                terrain.heights[worldIndex]
                            ) != std::bit_cast<std::uint32_t>(height)
                        ) {
                            throw std::runtime_error(
                                "Adjacent terrain chunks have different borders"
                            );
                        }
                    } else {
                        terrain.heights[worldIndex] = height;
                        loaded[worldIndex] = 1;
                    }
                }
            }
        }

    }

    void save_world(
        const Heightfield& terrain,
        const Settings& settings,
        const std::filesystem::path& directory
    ) {
        validate_world(terrain, settings);

        if (directory.empty() || directory.filename().empty()) {
            throw std::invalid_argument("Invalid world directory");
        }

        fs::path pending = directory;
        fs::path backup = directory;

        pending += ".pending";
        backup += ".backup";

        if (fs::exists(pending) || fs::exists(backup)) {
            throw std::runtime_error(
                "Pending or backup terrain world already exists"
            );
        }

        if (fs::exists(directory) && !fs::is_directory(directory)) {
            throw std::runtime_error(
                "World path exists and is not a directory"
            );
        }

        const fs::path parent = directory.parent_path();

        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        bool originalMoved = false;

        try {
            fs::create_directories(pending / "chunks");
            write_manifest(pending / "world.meta", settings);

            for (
                std::uint32_t z = 0;
                z + 1 < terrain.samplesZ;
                z += chunkCells
            ) {
                for (
                    std::uint32_t x = 0;
                    x + 1 < terrain.samplesX;
                    x += chunkCells
                ) {
                    write_chunk(pending, terrain, x, z);
                }
            }

            if (fs::exists(directory)) {
                fs::rename(directory, backup);
                originalMoved = true;
            }

            try {
                fs::rename(pending, directory);
            } catch (...) {
                if (originalMoved) {
                    fs::rename(backup, directory);
                }

                throw;
            }

            if (originalMoved) {
                fs::remove_all(backup);
            }
        } catch (...) {
            std::error_code error;
            fs::remove_all(pending, error);
            throw;
        }
    }

    WorldDocument load_world(
        const std::filesystem::path& directory
    ) {
        WorldDocument world;
        world.settings = read_manifest(directory / "world.meta");

        Heightfield& terrain = world.terrain;

        terrain.samplesX = world.settings.samplesX;
        terrain.samplesZ = world.settings.samplesZ;
        terrain.originX = world.settings.originX;
        terrain.originZ = world.settings.originZ;
        terrain.spacingMeters = world.settings.spacingMeters;

        terrain.heights.resize(
            static_cast<std::size_t>(terrain.samplesX) *
            terrain.samplesZ
        );

        std::vector<std::uint8_t> loaded(
            terrain.heights.size(),
            0
        );

        for (
            std::uint32_t z = 0;
            z + 1 < terrain.samplesZ;
            z += chunkCells
        ) {
            for (
                std::uint32_t x = 0;
                x + 1 < terrain.samplesX;
                x += chunkCells
            ) {
                read_chunk(directory, terrain, loaded, x, z);
            }
        }

        if (
            std::find(loaded.begin(), loaded.end(), 0) != loaded.end()
        ) {
            throw std::runtime_error(
                "Terrain world has missing samples"
            );
        }

        return world;
    }

     Settings read_world_settings(
        const std::filesystem::path& directory
    )
    {
        return read_manifest(directory / "world.meta");
    }

    WorldChunk read_world_chunk(
        const std::filesystem::path& directory,
        const Settings& settings,
        std::uint32_t chunkX,
        std::uint32_t chunkZ
    ) {
        validate_settings(settings);

        const std::uint32_t countX =
            (settings.samplesX - 2) / chunkCells + 1;

        const std::uint32_t countZ =
            (settings.samplesZ - 2) / chunkCells + 1;

        if (chunkX >= countX || chunkZ >= countZ) {
            throw std::out_of_range("World chunk is out of range");
        }

        const std::uint32_t startX = chunkX * chunkCells;
        const std::uint32_t startZ = chunkZ * chunkCells;

        const std::uint32_t columns = (std::min)(
            chunkCells,
            settings.samplesX - 1 - startX
        ) + 1;

        const std::uint32_t rows = (std::min)(
            chunkCells,
            settings.samplesZ - 1 - startZ
        ) + 1;

        std::ifstream input(
            chunk_path(directory, startX, startZ),
            std::ios::binary
        );

        if (!input) {
            throw std::runtime_error("Unable to open world chunk");
        }

        check_magic(input, chunkMagic);

        const std::uint32_t version = read_u32(input);
        const std::uint32_t storedX = read_u32(input);
        const std::uint32_t storedZ = read_u32(input);
        const std::uint32_t storedColumns = read_u32(input);
        const std::uint32_t storedRows = read_u32(input);
        const std::uint32_t storedChecksum = read_u32(input);

        if (version != formatVersion ||
            storedX != startX ||
            storedZ != startZ ||
            storedColumns != columns ||
            storedRows != rows) {
            throw std::runtime_error(
                "World chunk header does not match metadata"
            );
        }

        std::vector<std::uint8_t> payload(
            static_cast<std::size_t>(columns) * rows * sizeof(float)
        );

        read_bytes(
            input,
            reinterpret_cast<char*>(payload.data()),
            static_cast<std::streamsize>(payload.size())
        );

        check_end(input);

        if (checksum(payload) != storedChecksum) {
            throw std::runtime_error("World chunk checksum mismatch");
        }

        WorldChunk result;

        result.startX = startX;
        result.startZ = startZ;
        result.samplesX = columns;
        result.samplesZ = rows;

        result.originX =
            settings.originX + static_cast<float>(startX) * settings.spacingMeters;

        result.originZ =
            settings.originZ + static_cast<float>(startZ) * settings.spacingMeters;

        result.spacingMeters = settings.spacingMeters;
        result.heights.resize(static_cast<std::size_t>(columns) * rows);

        for (std::size_t i = 0; i < result.heights.size(); ++i) {
            const float height = unpack_float(
                payload,
                i * sizeof(float)
            );

            if (!std::isfinite(height)) {
                throw std::runtime_error(
                    "World chunk contains invalid heights"
                );
            }

            result.heights[i] = height;
        }

        return result;
    }

}
