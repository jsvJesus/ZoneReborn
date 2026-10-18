#pragma once

#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace world_editor::vegetation {

    enum class VegetationHabitat : std::uint8_t {
        Any,
        Grassland,
        Forest,
        Sandy,
        Rocky,
        Wetland
    };

    struct VegetationCatalogEntry {
        // Path relative to bin/packs.
        std::string relativePath;

        VegetationKind kind =
            VegetationKind::Unclassified;

        VegetationHabitat habitat =
            VegetationHabitat::Any;

        float weight = 1.0f;

        bool enabled = false;
        bool lodVariant = false;
    };

    struct VegetationAssetCatalog {
        std::vector<VegetationCatalogEntry> entries;

        [[nodiscard]] std::size_t enabled_count() const noexcept;
    };

    [[nodiscard]] VegetationAssetCatalog scan_vegetation_catalog(
        const std::filesystem::path& packsDirectory
    );

}
