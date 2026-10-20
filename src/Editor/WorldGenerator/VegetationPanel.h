#pragma once

#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"
#include "WorldEditor/Vegetation/VegetationGenerator.h"
#include "WorldEditor/WorldGenerator/MapGenerationSettings.h"
#include "Editor/WorldGenerator/LayerRevisions.h"
#include "Editor/WorldGenerator/VegetationCatalogStorage.h"

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
        ~VegetationPanel();

        void draw(
            const world_editor::terrain::Heightfield& terrain,
            const world_editor::biome::BiomeLayer* biomes,
            std::uint64_t biomeSourceFingerprint,
            viewport::TerrainViewport& viewport,
            const world_editor::world_generator::GenerationSeeds& seeds
        );

        void clear(
            viewport::TerrainViewport& viewport
        );

        void synchronize(
            viewport::TerrainViewport& viewport,
            const world_editor::world_generator::GenerationSeeds& seeds
        );

    private:
        LayerRevisions sourceRevisions_;
        void refresh_catalog(
            viewport::TerrainViewport& viewport
        );

        void invalidate(
            viewport::TerrainViewport& viewport
        );

        void persist_preferences();

        world_editor::vegetation::VegetationSettings pending_;

        world_editor::vegetation::VegetationAssetCatalog catalog_;
        VegetationCatalogPreferences preferences_;

        std::optional<
            world_editor::vegetation::VegetationLayer
        > active_;

        std::array<std::size_t, 3> counts_{};

        std::size_t usedVariants_ = 0;

        bool catalogInitialized_ = false;
        bool preferencesDirty_ = false;
        bool preferencesSaveFailed_ = false;
        bool showPreview_ = true;
        bool showGeometry_ = false;

        char assetFilter_[128]{};
        int selectedSource_ = 0;
        int selectedHabitat_ = 0;
        int selectedKind_ = 0;

        std::string status_;
        bool hasError_ = false;
    };

}
