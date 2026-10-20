#pragma once

#include "WorldEditor/Biome/BiomeGenerator.h"
#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapGenerationSettings.h"
#include "Editor/WorldGenerator/LayerRevisions.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::world_generator {

    class BiomeMaterialsPanel final {
    public:
        void draw(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::terrain::Settings& activeSettings,
            viewport::TerrainViewport& viewport,
            const world_editor::world_generator::GenerationSeeds& seeds
        );

        void generate(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::terrain::Settings& activeSettings,
            viewport::TerrainViewport& viewport,
            const world_editor::world_generator::GenerationSeeds& seeds
        );

        [[nodiscard]] world_editor::biome::BiomeSettings&
        generation_settings() noexcept {
            return pending_;
        }

        void clear(viewport::TerrainViewport& viewport);

        void synchronize(
            viewport::TerrainViewport& viewport,
            const world_editor::world_generator::GenerationSeeds& seeds,
            const world_editor::terrain::Settings& activeSettings
        );

        void validate_for_save(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::biome::HydrologyLayer* hydrology
        ) const;

        void save_for_world(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::terrain::Settings& settings,
            const world_editor::biome::HydrologyLayer* hydrology,
            const std::filesystem::path& directory
        ) const;

        [[nodiscard]] bool restore_from_world(
            const world_editor::terrain::Settings& settings,
            viewport::TerrainViewport& viewport,
            const std::filesystem::path& directory,
            const world_editor::terrain::Heightfield& terrain
        );

        [[nodiscard]] const world_editor::biome::BiomeLayer*
            active_layer() const noexcept {
            return active_ ? &*active_ : nullptr;
        }

        [[nodiscard]] std::uint64_t source_fingerprint() const noexcept {
            return sourceFingerprint_;
        }

    private:
        LayerRevisions sourceRevisions_;
        world_editor::biome::BiomeSettings pending_;
        std::optional<world_editor::biome::BiomeLayer> active_;

        std::uint64_t sourceFingerprint_ = 0;

        std::string status_;
        bool hasError_ = false;
    };

}
