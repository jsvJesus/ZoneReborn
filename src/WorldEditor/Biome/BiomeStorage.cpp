#include "WorldEditor/Biome/BiomeStorage.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <type_traits>

namespace world_editor::biome {

    namespace {

        constexpr std::array<char, 8> magic = {
            'Z', 'B', 'I', 'O', 'M', 'E', '0', '1'
        };

        constexpr std::uint32_t version = 1;
        constexpr std::uint64_t fnvOffset = 14695981039346656037ull;
        constexpr std::uint64_t fnvPrime = 1099511628211ull;

        static_assert(sizeof(BiomeKind) == 1);

        template<typename T>
        void write_value(
            std::ostream& stream,
            const T& value
        ) {
            static_assert(std::is_trivially_copyable_v<T>);

            stream.write(
                reinterpret_cast<const char*>(&value),
                sizeof(T)
            );

            if (!stream) {
                throw std::runtime_error(
                    "Failed to write Biome data"
                );
            }
        }

        template<typename T>
        void read_value(
            std::istream& stream,
            T& value
        ) {
            static_assert(std::is_trivially_copyable_v<T>);

            stream.read(
                reinterpret_cast<char*>(&value),
                sizeof(T)
            );

            if (!stream) {
                throw std::runtime_error(
                    "Invalid or truncated Biome file"
                );
            }
        }

        void hash_byte(
            std::uint64_t& hash,
            std::uint8_t value
        ) noexcept {
            hash ^= value;
            hash *= fnvPrime;
        }

        void hash_integer(
            std::uint64_t& hash,
            std::uint32_t value
        ) noexcept {
            for (int i = 0; i < 4; ++i) {
                hash_byte(
                    hash,
                    static_cast<std::uint8_t>(
                        (value >> (i * 8)) & 0xFFu
                    )
                );
            }
        }

        [[nodiscard]] std::uint64_t layer_checksum(
            const BiomeLayer& layer
        ) noexcept {
            std::uint64_t hash = fnvOffset;

            for (std::uint8_t value : layer.materialWeights) {
                hash_byte(hash, value);
            }

            for (BiomeKind kind : layer.zones) {
                hash_byte(
                    hash,
                    static_cast<std::uint8_t>(kind)
                );
            }

            return hash;
        }

        [[nodiscard]] bool matches_terrain(
            const BiomeLayer& layer,
            const terrain::Settings& settings
        ) noexcept {
            return layer.samplesX == settings.samplesX &&
                layer.samplesZ == settings.samplesZ &&
                layer.originX == settings.originX &&
                layer.originZ == settings.originZ &&
                layer.spacingMeters == settings.spacingMeters;
        }

    }

    std::filesystem::path biome_file_path(
        const std::filesystem::path& worldDirectory
    ) {
        auto path = worldDirectory;
        path += L".zbiome";
        return path;
    }

    std::uint64_t terrain_source_fingerprint(
        const terrain::Heightfield& heightfield
    ) {
        if (!heightfield.valid()) {
            throw std::invalid_argument(
                "Invalid Terrain for Biome fingerprint"
            );
        }

        std::uint64_t hash = fnvOffset;

        hash_integer(hash, heightfield.samplesX);
        hash_integer(hash, heightfield.samplesZ);

        for (float height : heightfield.heights) {
            hash_integer(
                hash,
                std::bit_cast<std::uint32_t>(height)
            );
        }

        return hash;
    }

    std::uint64_t biome_source_fingerprint(
        std::uint64_t terrainFingerprint,
        const HydrologyLayer* hydrology
    ) {
        if (hydrology && !hydrology->valid()) {
            throw std::invalid_argument(
                "Invalid Hydrology for Biome fingerprint"
            );
        }

        std::uint64_t hash = terrainFingerprint;
        hash_byte(hash, hydrology ? 1 : 0);

        if (hydrology) {
            for (WaterKind kind : hydrology->cells) {
                hash_byte(
                    hash,
                    static_cast<std::uint8_t>(kind)
                );
            }
        }

        return hash;
    }

    std::uint64_t biome_source_fingerprint(
        const terrain::Heightfield& heightfield,
        const HydrologyLayer* hydrology
    ) {
        if (hydrology &&
            (hydrology->samplesX != heightfield.samplesX ||
                hydrology->samplesZ != heightfield.samplesZ ||
                hydrology->originX != heightfield.originX ||
                hydrology->originZ != heightfield.originZ ||
                hydrology->spacingMeters != heightfield.spacingMeters)) {
            throw std::invalid_argument("Hydrology grid does not match Terrain");
        }

        return biome_source_fingerprint(
            terrain_source_fingerprint(heightfield),
            hydrology
        );
    }

    void save_biomes(
        const BiomeLayer& layer,
        const terrain::Settings& terrainSettings,
        std::uint64_t sourceFingerprint,
        const std::filesystem::path& worldDirectory
    ) {
        if (!layer.valid() ||
            !matches_terrain(layer, terrainSettings)) {
            throw std::invalid_argument(
                "Biome data does not match saved Terrain"
            );
        }

        const auto path = biome_file_path(worldDirectory);

        std::ofstream stream(
            path,
            std::ios::binary | std::ios::trunc
        );

        if (!stream) {
            throw std::runtime_error(
                "Failed to create Biome file"
            );
        }

        stream.write(magic.data(), magic.size());

        write_value(stream, version);
        write_value(stream, terrainSettings.seed);

        write_value(stream, layer.samplesX);
        write_value(stream, layer.samplesZ);
        write_value(stream, layer.originX);
        write_value(stream, layer.originZ);
        write_value(stream, layer.spacingMeters);

        write_value(stream, layer.settings.seed);
        write_value(stream, layer.settings.moisture);
        write_value(stream, layer.settings.forestCoverage);
        write_value(stream, layer.settings.rockSlopeDegrees);
        write_value(stream, layer.settings.textureScaleMeters);

        write_value(stream, sourceFingerprint);

        const std::uint64_t checksum =
            layer_checksum(layer);

        write_value(stream, checksum);

        stream.write(
            reinterpret_cast<const char*>(
                layer.materialWeights.data()
            ),
            static_cast<std::streamsize>(
                layer.materialWeights.size()
            )
        );

        stream.write(
            reinterpret_cast<const char*>(
                layer.zones.data()
            ),
            static_cast<std::streamsize>(
                layer.zones.size()
            )
        );

        if (!stream) {
            throw std::runtime_error(
                "Failed to save Biome material data"
            );
        }
    }

    SavedBiome load_biomes(
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& worldDirectory
    ) {
        std::ifstream stream(
            biome_file_path(worldDirectory),
            std::ios::binary
        );

        if (!stream) {
            throw std::runtime_error(
                "Biome file not found"
            );
        }

        std::array<char, 8> signature{};

        stream.read(signature.data(), signature.size());

        if (!stream || signature != magic) {
            throw std::runtime_error(
                "Invalid Biome file signature"
            );
        }

        std::uint32_t fileVersion = 0;
        std::uint32_t terrainSeed = 0;

        read_value(stream, fileVersion);
        read_value(stream, terrainSeed);

        if (fileVersion != version ||
            terrainSeed != terrainSettings.seed) {
            throw std::runtime_error(
                "Unsupported or outdated Biome file"
            );
        }

        SavedBiome result;
        BiomeLayer& layer = result.layer;

        read_value(stream, layer.samplesX);
        read_value(stream, layer.samplesZ);
        read_value(stream, layer.originX);
        read_value(stream, layer.originZ);
        read_value(stream, layer.spacingMeters);

        if (layer.samplesX < 2 ||
            layer.samplesZ < 2 ||
            layer.samplesX > 2049 ||
            layer.samplesZ > 2049 ||
            !matches_terrain(layer, terrainSettings)) {
            throw std::runtime_error(
                "Biome grid does not match world"
            );
        }

        read_value(stream, layer.settings.seed);
        read_value(stream, layer.settings.moisture);
        read_value(stream, layer.settings.forestCoverage);
        read_value(stream, layer.settings.rockSlopeDegrees);
        read_value(stream, layer.settings.textureScaleMeters);

        if (!std::isfinite(layer.settings.moisture) ||
            !std::isfinite(layer.settings.forestCoverage) ||
            !std::isfinite(layer.settings.rockSlopeDegrees) ||
            !std::isfinite(layer.settings.textureScaleMeters) ||
            layer.settings.moisture < 0.0f ||
            layer.settings.moisture > 1.0f ||
            layer.settings.forestCoverage < 0.0f ||
            layer.settings.forestCoverage > 1.0f ||
            layer.settings.rockSlopeDegrees < 5.0f ||
            layer.settings.rockSlopeDegrees > 60.0f ||
            layer.settings.textureScaleMeters <= 0.0f) {
            throw std::runtime_error(
                "Invalid saved Biome settings"
            );
        }

        read_value(stream, result.sourceFingerprint);

        std::uint64_t expectedChecksum = 0;
        read_value(stream, expectedChecksum);

        const std::size_t count =
            static_cast<std::size_t>(layer.samplesX) *
            layer.samplesZ;

        layer.materialWeights.resize(count * 4);
        layer.zones.resize(count);

        stream.read(
            reinterpret_cast<char*>(
                layer.materialWeights.data()
            ),
            static_cast<std::streamsize>(
                layer.materialWeights.size()
            )
        );

        stream.read(
            reinterpret_cast<char*>(
                layer.zones.data()
            ),
            static_cast<std::streamsize>(
                layer.zones.size()
            )
        );

        if (!stream || layer_checksum(layer) != expectedChecksum) {
            throw std::runtime_error(
                "Biome file checksum mismatch"
            );
        }

        for (BiomeKind kind : layer.zones) {
            if (static_cast<std::uint8_t>(kind) >
                static_cast<std::uint8_t>(
                    BiomeKind::Wetland
                )) {
                throw std::runtime_error(
                    "Invalid saved Biome kind"
                );
            }
        }

        char extra = 0;

        if (stream.read(&extra, 1) || !stream.eof()) {
            throw std::runtime_error(
                "Invalid trailing Biome data"
            );
        }

        return result;
    }

}
