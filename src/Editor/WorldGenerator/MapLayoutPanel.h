#pragma once

#include "WorldEditor/WorldGenerator/MapLayoutGenerator.h"
#include "WorldEditor/Biome/Hydrology.h"

#include <optional>
#include <string>
#include <filesystem>

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::world_generator {

    class WorldStoragePanel;

    class MapLayoutPanel final {
    public:
        MapLayoutPanel();

        void draw(
            world_editor::terrain::Heightfield& terrain,
            world_editor::terrain::Settings& settings,
            world_editor::terrain::Settings& activeSettings,
            viewport::TerrainViewport& viewport,
            const WorldStoragePanel& storage
        );

        void draw_inspector() const;

        void clear(viewport::TerrainViewport& viewport);

        void save_hydrology_for_world(
            const world_editor::terrain::Heightfield& heightfield,
            const world_editor::terrain::Settings& settings,
            const std::filesystem::path& directory
        ) const;

        [[nodiscard]] bool restore_hydrology(
            const world_editor::terrain::Heightfield& heightfield,
            const world_editor::terrain::Settings& settings,
            viewport::TerrainViewport& viewport,
            const std::filesystem::path& directory
        );

    private:
        world_editor::world_generator::MapGenerationSettings settings_;
        std::optional<world_editor::world_generator::MapLayout> layout_;
        std::optional<world_editor::terrain::Settings> sourceTerrain_;

        int poiPreset_ = 0;
        std::string status_;
        bool hasError_ = false;

        world_editor::biome::HydrologySettings hydrologySettings_;
        std::optional<world_editor::biome::HydrologyLayer> hydrology_;
        std::optional<world_editor::terrain::Settings> sourceHydrologyTerrain_;
    };

}
