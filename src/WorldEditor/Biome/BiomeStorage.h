#pragma once

#include "WorldEditor/Biome/BiomeGenerator.h"

#include <cstdint>
#include <filesystem>

namespace world_editor::biome {

    struct SavedBiome {
        BiomeLayer layer;
        std::uint64_t sourceFingerprint = 0;
    };

    [[nodiscard]] std::filesystem::path biome_file_path(
        const std::filesystem::path& worldDirectory
    );

    [[nodiscard]] std::uint64_t terrain_source_fingerprint(
        const terrain::Heightfield& heightfield
    );

    [[nodiscard]] std::uint64_t biome_source_fingerprint(
        std::uint64_t terrainFingerprint,
        const HydrologyLayer* hydrology
    );

    [[nodiscard]] std::uint64_t biome_source_fingerprint(
        const terrain::Heightfield& heightfield,
        const HydrologyLayer* hydrology
    );

    void save_biomes(
        const BiomeLayer& layer,
        const terrain::Settings& terrainSettings,
        std::uint64_t sourceFingerprint,
        const std::filesystem::path& worldDirectory
    );

    [[nodiscard]] SavedBiome load_biomes(
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& worldDirectory
    );

}
