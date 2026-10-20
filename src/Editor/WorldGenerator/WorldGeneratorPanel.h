#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <string>

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::ui {
    class OutputLog;
}

namespace editor::world_generator {

    class MapLayoutPanel;
    class BiomeMaterialsPanel;
    class TerrainReliefPanel;

    class WorldGeneratorPanel final {
    public:
        void draw(
            world_editor::terrain::Heightfield& terrain,
            world_editor::terrain::Settings& settings,
            world_editor::terrain::Settings& activeSettings,
            viewport::TerrainViewport& viewport,
            MapLayoutPanel& mapPanel,
            BiomeMaterialsPanel& biomePanel,
            TerrainReliefPanel& reliefPanel,
            ui::OutputLog& outputLog
        );

    private:
        bool terrainEnabled_ = true;
        bool hydrologyEnabled_ = true;
        bool layoutEnabled_ = true;
        bool biomesEnabled_ = true;
        std::string status_;
        bool hasError_ = false;
    };

}
