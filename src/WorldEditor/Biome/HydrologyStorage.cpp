#include "WorldEditor/Biome/HydrologyStorage.h"

#include <Windows.h>

#include <bit>
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
        constexpr std::uint32_t fileVersion = 1;
        constexpr std::uint32_t algorithmVersion = 2;

        struct Writer {
            std::vector<std::uint8_t> data;

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
        };

        struct Reader {
            const std::vector<std::uint8_t>& data;
            std::size_t limit;
            std::size_t offset = 0;

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
        };

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
            fs::file_size(path) > 256 ||
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

        if (input.u32() != magic ||
            input.u32() != fileVersion ||
            input.u32() != algorithmVersion) {
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

        if (input.offset != payloadSize) {
            throw std::runtime_error(
                "Unexpected Hydrology file data"
            );
        }

        return generate_hydrology(heightfield, settings);
    }

}
