#pragma once

#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace editor::world_generator {

    struct VegetationModelPreference {
        world_editor::vegetation::VegetationAssetSource source =
            world_editor::vegetation::VegetationAssetSource::Flora;
        world_editor::vegetation::VegetationKind kind =
            world_editor::vegetation::VegetationKind::Unclassified;
        std::uint8_t biomeMask = 0;
        float weight = 1.0f;
        bool enabled = false;
    };

    struct VegetationCatalogPreferences {
        world_editor::vegetation::VegetationAssetSource source =
            world_editor::vegetation::VegetationAssetSource::SpeedTree;
        std::unordered_map<std::string, VegetationModelPreference> models;

        void remember(
            const world_editor::vegetation::VegetationCatalogEntry& entry
        );

        void apply(
            world_editor::vegetation::VegetationAssetCatalog& catalog
        ) const;
    };

    [[nodiscard]] std::optional<VegetationCatalogPreferences>
    load_vegetation_preferences(const std::filesystem::path& path);

    void save_vegetation_preferences(
        const VegetationCatalogPreferences& preferences,
        const std::filesystem::path& path
    );

}
