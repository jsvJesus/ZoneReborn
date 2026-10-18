#pragma once

#include "WorldEditor/Biome/BiomeGenerator.h"
#include "WorldEditor/Terrain/TerrainData.h"

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
            viewport::TerrainViewport& viewport
        );

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
            const world_editor::terrain::Heightfield* terrain = nullptr
        );

        [[nodiscard]] const world_editor::biome::BiomeLayer*
            active_layer() const noexcept {
            return active_ ? &*active_ : nullptr;
        }

        [[nodiscard]] std::uint64_t source_fingerprint() const noexcept {
            return sourceFingerprint_;
        }

    private:
        world_editor::biome::BiomeSettings pending_;
        std::optional<world_editor::biome::BiomeLayer> active_;

        std::uint64_t sourceFingerprint_ = 0;

        std::string status_;
        bool hasError_ = false;
    };

}
