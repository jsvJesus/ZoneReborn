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
        bool terrainEnabled_ = false;
        bool hydrologyEnabled_ = false;
        bool layoutEnabled_ = false;
        bool biomesEnabled_ = false;
        std::string status_;
        bool hasError_ = false;
    };

}
