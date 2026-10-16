
#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <string>

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::world_generator {

    class WorldStoragePanel final {
    public:
        void draw(
            world_editor::terrain::Heightfield& terrain,
            world_editor::terrain::Settings& settings,
            world_editor::terrain::Settings& activeSettings,
            viewport::TerrainViewport& viewport
        );

    private:
        char worldName_[65] = "polissya_test";
        std::string status_;
        bool hasError_ = false;
    };

}
