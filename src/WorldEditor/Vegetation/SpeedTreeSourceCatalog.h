#pragma once

#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"

#include <filesystem>
#include <string>
#include <vector>

namespace world_editor::vegetation {

    struct SpeedTreeSource {
        std::string relativePath;
        std::string meshRelativePath;
        VegetationKind kind = VegetationKind::Tree;
        VegetationHabitat habitat = VegetationHabitat::Forest;
        bool compiledTree = false;
        bool renderableMesh = false;
    };

    [[nodiscard]] std::vector<SpeedTreeSource> scan_speedtree_sources(
        const std::filesystem::path& packsDirectory
    );

}
