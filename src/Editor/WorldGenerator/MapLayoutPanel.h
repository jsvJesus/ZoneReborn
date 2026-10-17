#pragma once

#include "WorldEditor/WorldGenerator/MapLayoutGenerator.h"

#include <optional>
#include <string>

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

    private:
        world_editor::world_generator::MapGenerationSettings settings_;
        std::optional<world_editor::world_generator::MapLayout> layout_;
        std::optional<world_editor::terrain::Settings> sourceTerrain_;

        int poiPreset_ = 0;
        std::string status_;
        bool hasError_ = false;
    };

}
