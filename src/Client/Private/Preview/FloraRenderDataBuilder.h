#pragma once

#include "Graphics/SceneRenderData.h"
#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Flora/FloraVisualAsset.h"

#include <cstddef>
#include <string>
#include <vector>

namespace client::preview
{
    struct FloraRenderData final
    {
        std::vector<std::size_t>
            meshIndices;

        std::size_t triangleCount =
            0;

        std::size_t texturedPrimitiveGroups =
            0;
    };

    class FloraRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::world::flora::FloraVisualAsset& asset,
            float alphaCutoff,
            graphics::SceneRenderData& scene,
            FloraRenderData& output,
            std::string& error);

    private:
        ModelRenderDataBuilder
            materialBuilder_;
    };
}