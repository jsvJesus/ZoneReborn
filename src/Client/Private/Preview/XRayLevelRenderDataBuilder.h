#pragma once

#include "Core/World/XRay/XRayLevelData.h"

#include "Graphics/Renderer.h"

#include <cstddef>
#include <string>

namespace client::preview
{
    struct XRayLevelRenderStatistics final
    {
        std::size_t staticVisualCount = 0;
        std::size_t hierarchyVisualCount = 0;
        std::size_t skippedVisualCount = 0;

        std::size_t meshCount = 0;
        std::size_t vertexCount = 0;
        std::size_t triangleCount = 0;

        std::size_t materialGroupCount = 0;
        std::size_t texturedMaterialCount = 0;
        std::size_t lightmappedMaterialCount = 0;
        std::size_t loadedTextureCount = 0;
        std::size_t missingTextureCount = 0;
    };

    [[nodiscard]]
    bool StreamXRayLevelRenderData(
        const core::world::xray::LevelData& level,
        graphics::Renderer& renderer,
        XRayLevelRenderStatistics& statistics,
        std::string& error);
}
