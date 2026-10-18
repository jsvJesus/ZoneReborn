#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets {

    struct VisualMaterial {
        std::uint32_t primitiveGroup = 0;

        std::string identifier;
        std::filesystem::path diffusePath;

        bool alphaTest = false;
        bool doubleSided = false;

        std::uint8_t alphaReference = 0;
    };

    struct VisualMaterialSet {
        std::filesystem::path modelPath;
        std::filesystem::path visualPath;

        std::vector<VisualMaterial> materials;
    };

    [[nodiscard]] VisualMaterialSet load_visual_materials(
        const std::filesystem::path& packsDirectory,
        const std::filesystem::path& primitivesRelativePath
    );

}
