#pragma once

#include "Engine/Assets/PrimitivesMeshLoader.h"
#include "Engine/Assets/VisualMaterialLoader.h"

#include <filesystem>

namespace engine::assets {

    struct CompiledTreeModel {
        PrimitivesMesh mesh;
        VisualMaterialSet materials;
        bool cameraFacing = false;
    };

    [[nodiscard]] CompiledTreeModel load_compiled_tree(
        const std::filesystem::path& packsDirectory,
        const std::filesystem::path& sourceRelativePath
    );

}
