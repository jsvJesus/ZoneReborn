#pragma once

#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include <array>
#include <optional>
#include <string>

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::world_generator {

    class VegetationPanel final {
    public:
        void draw(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::biome::BiomeLayer* biomes,
            std::uint64_t biomeSourceFingerprint,
            viewport::TerrainViewport& viewport
        );

        void clear(
            viewport::TerrainViewport& viewport
        );

    private:
        world_editor::vegetation::VegetationSettings pending_;

        std::optional<
            world_editor::vegetation::VegetationLayer
        > active_;

        std::array<std::size_t, 3> counts_{};

        bool useLayoutSeed_ = true;
        bool showPreview_ = true;

        std::string status_;
        bool hasError_ = false;
    };

}
