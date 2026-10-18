#pragma once

#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"
#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include <array>
#include <cstddef>
#include <cstdint>
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
        void refresh_catalog(
            viewport::TerrainViewport& viewport
        );

        void invalidate(
            viewport::TerrainViewport& viewport
        );

        world_editor::vegetation::VegetationSettings pending_;

        world_editor::vegetation::VegetationAssetCatalog catalog_;

        std::optional<
            world_editor::vegetation::VegetationLayer
        > active_;

        std::array<std::size_t, 3> counts_{};

        std::size_t usedVariants_ = 0;

        bool catalogInitialized_ = false;
        bool useLayoutSeed_ = true;
        bool showPreview_ = true;
        bool showGeometry_ = false;

        char assetFilter_[128]{};

        std::string status_;
        bool hasError_ = false;
    };

}
