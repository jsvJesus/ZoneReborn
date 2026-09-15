#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/World/WorldScene.h"

#include <cstddef>
#include <string>

namespace client::preview
{
    struct WaterRenderData final
    {
        std::size_t meshIndex =
            0;

        std::size_t materialIndex =
            0;

        std::size_t vertexCount =
            0;

        std::size_t triangleCount =
            0;
    };

    class WaterRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::world::WorldLargeObjectReference& object,
            graphics::SceneRenderData& scene,
            WaterRenderData& output,
            std::string& error) const;
    };
}