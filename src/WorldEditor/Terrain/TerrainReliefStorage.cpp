#include "WorldEditor/Terrain/TerrainReliefStorage.h"

#include <Windows.h>

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace world_editor::terrain {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::uint32_t magic = 0x464C525Au;
        constexpr std::uint32_t version = 1;
        constexpr std::uint32_t algorithmVersion = 1;

        constexpr std::size_t fileSize = 44;

        struct Writer {
            std::vector<std::uint8_t> data;

            void u32(std::uint32_t value) {
                for (int i = 0; i < 4; ++i) {
                    data.push_back(
                        static_cast<std::uint8_t>(
                            (value >> (i * 8)) & 0xFFu
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
            std::size_t offset = 0;

            [[nodiscard]] std::uint32_t u32() {
                if (offset > data.size() ||
                    data.size() - offset < 4) {
                    throw std::runtime_error(
                        "Incomplete Terrain Relief file"
                    );
                }

                std::uint32_t value = 0;

                for (int i = 0; i < 4; ++i) {
                    value |= static_cast<std::uint32_t>(
                        data[offset++]
                    ) << (i * 8);
                }

                return value;
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
            std::size_t count
        ) noexcept {
            std::uint32_t hash = 2166136261u;

            for (std::size_t i = 0; i < count; ++i) {
                hash = (hash ^ bytes[i]) * 16777619u;
            }

            return hash;
        }

        void hash_u32(
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

        [[nodiscard]] std::uint64_t fingerprint(
            const Heightfield& terrain
        ) {
            if (!terrain.valid()) {
                throw std::invalid_argument(
                    "Invalid Terrain for Relief storage"
                );
            }

            std::uint64_t hash = 14695981039346656037ull;

            hash_u32(hash, terrain.samplesX);
            hash_u32(hash, terrain.samplesZ);

            hash_u32(
                hash,
                std::bit_cast<std::uint32_t>(
                    terrain.originX
                )
            );

            hash_u32(
                hash,
                std::bit_cast<std::uint32_t>(
                    terrain.originZ
                )
            );

            hash_u32(
                hash,
                std::bit_cast<std::uint32_t>(
                    terrain.spacingMeters
                )
            );

            for (const float height : terrain.heights) {
                hash_u32(
                    hash,
                    std::bit_cast<std::uint32_t>(height)
                );
            }

            return hash;
        }

    }

    void save_relief_profile(
        const ReliefProfile& profile,
        const Heightfield& heightfield,
        const fs::path& path
    ) {
        if (path.empty()) {
            throw std::invalid_argument(
                "Terrain Relief path is empty"
            );
        }

        Writer output;

        output.u32(magic);
        output.u32(version);
        output.u32(algorithmVersion);
        output.u64(fingerprint(heightfield));

        output.u32(static_cast<std::uint32_t>(
            profile.preset
        ));

        output.f32(profile.hillHeightMeters);
        output.f32(profile.mountainHeightMeters);
        output.f32(profile.mountainCoverage);
        output.f32(profile.ridgeSharpness);

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
                "Pending Relief save already exists"
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
                        "Cannot create Terrain Relief file"
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
                        "Cannot write Terrain Relief file"
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
                    "Cannot replace Terrain Relief file"
                );
            }
        } catch (...) {
            std::error_code error;
            fs::remove(pending, error);
            throw;
        }
    }

    ReliefProfile load_relief_profile(
        const Heightfield& heightfield,
        const fs::path& path
    ) {
        if (!fs::exists(path) ||
            fs::file_size(path) != fileSize) {
            throw std::runtime_error(
                "Invalid Terrain Relief file size"
            );
        }

        std::ifstream file(path, std::ios::binary);

        if (!file) {
            throw std::runtime_error(
                "Cannot open Terrain Relief file"
            );
        }

        const std::vector<std::uint8_t> data(
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        );

        if (data.size() != fileSize) {
            throw std::runtime_error(
                "Incomplete Terrain Relief file"
            );
        }

        const std::size_t payload = fileSize - 4;

        Reader footer{data, payload};
        const std::uint32_t storedChecksum = footer.u32();

        if (checksum(data, payload) != storedChecksum) {
            throw std::runtime_error(
                "Terrain Relief checksum mismatch"
            );
        }

        Reader reader{data};

        if (reader.u32() != magic ||
            reader.u32() != version ||
            reader.u32() != algorithmVersion) {
            throw std::runtime_error(
                "Unsupported Terrain Relief version"
            );
        }

        if (reader.u64() != fingerprint(heightfield)) {
            throw std::runtime_error(
                "Relief profile belongs to another Terrain"
            );
        }

        ReliefProfile profile;

        profile.preset = static_cast<ReliefPreset>(
            reader.u32()
        );

        profile.hillHeightMeters = reader.f32();
        profile.mountainHeightMeters = reader.f32();
        profile.mountainCoverage = reader.f32();
        profile.ridgeSharpness = reader.f32();

        // Validate stored parameters through the generator.
        // The full Terrain need not be generated here:
        // call the shared validator in the next revision.
        if (static_cast<std::uint32_t>(profile.preset) > 4 ||
            !std::isfinite(profile.hillHeightMeters) ||
            !std::isfinite(profile.mountainHeightMeters) ||
            !std::isfinite(profile.mountainCoverage) ||
            !std::isfinite(profile.ridgeSharpness) ||
            profile.hillHeightMeters < 0.0f ||
            profile.hillHeightMeters > 300.0f ||
            profile.mountainHeightMeters < 0.0f ||
            profile.mountainHeightMeters > 1200.0f ||
            profile.mountainCoverage < 0.0f ||
            profile.mountainCoverage > 1.0f ||
            profile.ridgeSharpness < 0.5f ||
            profile.ridgeSharpness > 5.0f) {
            throw std::runtime_error(
                "Invalid stored Terrain Relief settings"
            );
        }

        return profile;
    }

}
