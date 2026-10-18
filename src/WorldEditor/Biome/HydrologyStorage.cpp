#include "WorldEditor/Biome/HydrologyStorage.h"

#include <Windows.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace world_editor::biome {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::uint32_t magic = 0x4459485Au;
        constexpr std::uint32_t fileVersion = 2;
        constexpr std::uint32_t algorithmVersion = 2;
        constexpr std::uintmax_t maxFileSize = 16u * 1024u * 1024u;
        constexpr std::uint32_t maxWaterPoints = 500000;

        struct Writer {
            std::vector<std::uint8_t> data;

            void u8(const std::uint8_t value) {
                data.push_back(value);
            }

            void u32(std::uint32_t value) {
                for (int i = 0; i < 4; ++i) {
                    data.push_back(
                        static_cast<std::uint8_t>(
                            (value >> (8 * i)) & 0xFFu
                        )
                    );
                }
            }

            void u64(std::uint64_t value) {
                u32(static_cast<std::uint32_t>(value));
                u32(static_cast<std::uint32_t>(value >> 32));
            }

            void f32(float value) {
                u32(std::bit_cast<std::uint32_t>(value));
            }

            void point(const WaterPoint& value) {
                f32(value.x);
                f32(value.y);
                f32(value.z);
            }
        };

        struct Reader {
            const std::vector<std::uint8_t>& data;
            std::size_t limit;
            std::size_t offset = 0;

            [[nodiscard]] std::uint8_t u8() {
                if (offset >= limit) {
                    throw std::runtime_error("Hydrology file is incomplete");
                }

                return data[offset++];
            }

            [[nodiscard]] std::uint32_t u32() {
                if (offset > limit ||
                    limit - offset < 4) {
                    throw std::runtime_error(
                        "Hydrology file is incomplete"
                    );
                }

                std::uint32_t result = 0;

                for (int i = 0; i < 4; ++i) {
                    result |= static_cast<std::uint32_t>(
                        data[offset++]
                    ) << (8 * i);
                }

                return result;
            }

            [[nodiscard]] std::uint64_t u64() {
                const std::uint64_t low = u32();
                const std::uint64_t high = u32();

                return low | (high << 32);
            }

            [[nodiscard]] float f32() {
                return std::bit_cast<float>(u32());
            }

            [[nodiscard]] WaterPoint point() {
                const WaterPoint value{f32(), f32(), f32()};

                if (!std::isfinite(value.x) ||
                    !std::isfinite(value.y) ||
                    !std::isfinite(value.z)) {
                    throw std::runtime_error("Invalid Hydrology point");
                }

                return value;
            }
        };

        void write_points(
            Writer& output,
            const std::vector<WaterPoint>& points
        ) {
            if (points.size() > maxWaterPoints) {
                throw std::invalid_argument("Too many Hydrology points");
            }

            output.u32(static_cast<std::uint32_t>(points.size()));

            for (const auto& point : points) {
                output.point(point);
            }
        }

        [[nodiscard]] std::vector<WaterPoint> read_points(Reader& input) {
            const auto count = input.u32();

            if (count > maxWaterPoints ||
                count > (input.limit - input.offset) / 12u) {
                throw std::runtime_error("Invalid Hydrology point count");
            }

            std::vector<WaterPoint> points;
            points.reserve(count);

            for (std::uint32_t i = 0; i < count; ++i) {
                points.push_back(input.point());
            }

            return points;
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

        void mix(
            std::uint64_t& hash,
            const std::uint32_t value
        ) noexcept {
            for (int i = 0; i < 4; ++i) {
                hash ^= static_cast<std::uint8_t>(
                    (value >> (i * 8)) & 0xFFu
                );

                hash *= 1099511628211ull;
            }
        }

        [[nodiscard]] std::uint64_t terrain_fingerprint(
            const terrain::Heightfield& heightfield,
            const terrain::Settings& settings
        ) {
            if (!heightfield.valid()) {
                throw std::invalid_argument(
                    "Invalid Terrain for Hydrology"
                );
            }

            std::uint64_t hash = 14695981039346656037ull;

            mix(hash, settings.samplesX);
            mix(hash, settings.samplesZ);
            mix(hash, settings.seed);

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.originX
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.originZ
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.spacingMeters
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.baseHeightMeters
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.reliefMeters
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.macroScaleMeters
            ));

            mix(hash, std::bit_cast<std::uint32_t>(
                settings.detailScaleMeters
            ));

            for (const float height : heightfield.heights) {
                mix(
                    hash,
                    std::bit_cast<std::uint32_t>(height)
                );
            }

            return hash;
        }

    }

    void save_hydrology(
        const HydrologyLayer& water,
        const terrain::Heightfield& heightfield,
        const terrain::Settings& terrainSettings,
        const fs::path& path
    ) {
        if (!water.valid() ||
            !heightfield.valid() ||
            path.empty() ||
            water.samplesX != heightfield.samplesX ||
            water.samplesZ != heightfield.samplesZ ||
            water.originX != heightfield.originX ||
            water.originZ != heightfield.originZ ||
            water.spacingMeters != heightfield.spacingMeters ||
            terrainSettings.samplesX != heightfield.samplesX ||
            terrainSettings.samplesZ != heightfield.samplesZ ||
            terrainSettings.originX != heightfield.originX ||
            terrainSettings.originZ != heightfield.originZ ||
            terrainSettings.spacingMeters !=
                heightfield.spacingMeters) {
            throw std::invalid_argument(
                "Hydrology does not match active Terrain"
            );
        }

        Writer output;

        output.u32(magic);
        output.u32(fileVersion);
        output.u32(algorithmVersion);

        output.u64(terrain_fingerprint(
            heightfield,
            terrainSettings
        ));

        output.u32(water.samplesX);
        output.u32(water.samplesZ);
        output.f32(water.originX);
        output.f32(water.originZ);
        output.f32(water.spacingMeters);

        const auto& settings = water.settings;

        output.u32(settings.seed);
        output.u32(settings.riverEnabled ? 1u : 0u);
        output.u32(static_cast<std::uint32_t>(
            settings.riverShape
        ));

        output.u32(settings.lakeCount);
        output.f32(settings.riverWidthMeters);
        output.f32(settings.marshWidthMeters);
        output.f32(settings.lakeSeparationMeters);

        output.u32(static_cast<std::uint32_t>(water.cells.size()));

        for (const auto kind : water.cells) {
            output.u8(static_cast<std::uint8_t>(kind));
        }

        write_points(output, water.river);

        if (water.tributaries.size() > maxWaterPoints) {
            throw std::invalid_argument("Too many Hydrology tributaries");
        }

        output.u32(static_cast<std::uint32_t>(water.tributaries.size()));

        for (const auto& tributary : water.tributaries) {
            write_points(output, tributary);
        }

        if (water.lakes.size() > maxWaterPoints) {
            throw std::invalid_argument("Too many Hydrology lakes");
        }

        output.u32(static_cast<std::uint32_t>(water.lakes.size()));

        for (const auto& lake : water.lakes) {
            output.point(lake.center);
            output.f32(lake.radiusMeters);
        }

        write_points(output, water.marshMarkers);

        if (output.data.size() + 4u > maxFileSize) {
            throw std::invalid_argument("Hydrology file is too large");
        }

        output.u32(checksum(
            output.data,
            output.data.size()
        ));

        const fs::path parent = path.parent_path();

        if (!parent.empty()) {
            fs::create_directories(parent);
        }

        fs::path pending = path;
        pending += L".pending";

        if (fs::exists(pending)) {
            throw std::runtime_error(
                "Pending Hydrology save already exists"
            );
        }

        try {
            {
                std::ofstream file(
                    pending,
                    std::ios::binary | std::ios::trunc
                );

                if (!file) {
                    throw std::runtime_error(
                        "Cannot create Hydrology file"
                    );
                }

                file.write(
                    reinterpret_cast<const char*>(
                        output.data.data()
                    ),
                    static_cast<std::streamsize>(
                        output.data.size()
                    )
                );

                file.flush();

                if (!file) {
                    throw std::runtime_error(
                        "Cannot finish Hydrology save"
                    );
                }
            }

            if (!MoveFileExW(
                pending.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING |
                MOVEFILE_WRITE_THROUGH
            )) {
                throw std::runtime_error(
                    "Cannot replace Hydrology file"
                );
            }
        } catch (...) {
            std::error_code error;
            fs::remove(pending, error);
            throw;
        }
    }

    HydrologyLayer load_hydrology(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& terrainSettings,
        const fs::path& path
    ) {
        if (!fs::exists(path) ||
            fs::file_size(path) > maxFileSize ||
            fs::file_size(path) < 64) {
            throw std::runtime_error(
                "Hydrology file is missing or invalid"
            );
        }

        std::ifstream file(path, std::ios::binary);

        if (!file) {
            throw std::runtime_error(
                "Cannot open Hydrology file"
            );
        }

        const std::vector<std::uint8_t> data(
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        );

        if (data.size() < 64) {
            throw std::runtime_error(
                "Incomplete Hydrology file"
            );
        }

        const std::size_t payloadSize = data.size() - 4;

        const std::uint32_t savedChecksum =
            static_cast<std::uint32_t>(data[payloadSize]) |
            (static_cast<std::uint32_t>(
                data[payloadSize + 1]
            ) << 8) |
            (static_cast<std::uint32_t>(
                data[payloadSize + 2]
            ) << 16) |
            (static_cast<std::uint32_t>(
                data[payloadSize + 3]
            ) << 24);

        if (checksum(data, payloadSize) != savedChecksum) {
            throw std::runtime_error(
                "Hydrology checksum mismatch"
            );
        }

        Reader input{data, payloadSize};

        const auto savedMagic = input.u32();
        const auto savedVersion = input.u32();
        const auto savedAlgorithm = input.u32();

        if (savedMagic != magic ||
            (savedVersion != 1 && savedVersion != fileVersion) ||
            savedAlgorithm != algorithmVersion) {
            throw std::runtime_error(
                "Unsupported Hydrology file version"
            );
        }

        const std::uint64_t savedFingerprint = input.u64();

        if (savedFingerprint != terrain_fingerprint(
            heightfield,
            terrainSettings
        )) {
            throw std::runtime_error(
                "Hydrology belongs to a different Terrain"
            );
        }

        const std::uint32_t samplesX = input.u32();
        const std::uint32_t samplesZ = input.u32();

        const float originX = input.f32();
        const float originZ = input.f32();
        const float spacing = input.f32();

        if (samplesX != heightfield.samplesX ||
            samplesZ != heightfield.samplesZ ||
            originX != heightfield.originX ||
            originZ != heightfield.originZ ||
            spacing != heightfield.spacingMeters) {
            throw std::runtime_error(
                "Hydrology grid does not match Terrain"
            );
        }

        HydrologySettings settings;

        settings.seed = input.u32();

        const auto enabled = input.u32();

        if (enabled > 1) {
            throw std::runtime_error(
                "Invalid river enabled flag"
            );
        }

        settings.riverEnabled = enabled != 0;

        settings.riverShape = static_cast<RiverShape>(
            input.u32()
        );

        settings.lakeCount = input.u32();
        settings.riverWidthMeters = input.f32();
        settings.marshWidthMeters = input.f32();
        settings.lakeSeparationMeters = input.f32();

        if (savedVersion == 1) {
            if (input.offset != payloadSize) {
                throw std::runtime_error("Unexpected Hydrology file data");
            }

            return generate_hydrology(heightfield, settings);
        }

        HydrologyLayer result;
        result.samplesX = samplesX;
        result.samplesZ = samplesZ;
        result.originX = originX;
        result.originZ = originZ;
        result.spacingMeters = spacing;
        result.settings = settings;

        const auto cellCount = input.u32();

        if (cellCount != static_cast<std::uint64_t>(samplesX) * samplesZ ||
            cellCount > input.limit - input.offset) {
            throw std::runtime_error("Invalid Hydrology cell count");
        }

        result.cells.reserve(cellCount);

        for (std::uint32_t i = 0; i < cellCount; ++i) {
            const auto kind = input.u8();

            if (kind > static_cast<std::uint8_t>(WaterKind::Marsh)) {
                throw std::runtime_error("Invalid Hydrology water kind");
            }

            result.cells.push_back(static_cast<WaterKind>(kind));
        }

        result.river = read_points(input);
        const auto tributaryCount = input.u32();

        if (tributaryCount > maxWaterPoints ||
            tributaryCount > (input.limit - input.offset) / 4u) {
            throw std::runtime_error("Invalid Hydrology tributary count");
        }

        result.tributaries.reserve(tributaryCount);

        for (std::uint32_t i = 0; i < tributaryCount; ++i) {
            result.tributaries.push_back(read_points(input));
        }

        const auto lakeCount = input.u32();

        if (lakeCount > maxWaterPoints ||
            lakeCount > (input.limit - input.offset) / 16u) {
            throw std::runtime_error("Invalid Hydrology lake count");
        }

        result.lakes.reserve(lakeCount);

        for (std::uint32_t i = 0; i < lakeCount; ++i) {
            LakeArea lake;
            lake.center = input.point();
            lake.radiusMeters = input.f32();

            if (!std::isfinite(lake.radiusMeters) ||
                lake.radiusMeters < 0.0f) {
                throw std::runtime_error("Invalid Hydrology lake radius");
            }

            result.lakes.push_back(lake);
        }

        result.marshMarkers = read_points(input);

        if (input.offset != payloadSize) {
            throw std::runtime_error(
                "Unexpected Hydrology file data"
            );
        }

        if (!result.valid()) {
            throw std::runtime_error("Invalid Hydrology layer");
        }

        return result;
    }

}
