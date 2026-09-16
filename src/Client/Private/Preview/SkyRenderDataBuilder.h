#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Sky/SkyDefinition.h"

#include "Graphics/SceneRenderData.h"

#include <string>

namespace client::preview
{
    class SkyRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::world::sky::SkyDefinition& definition,
            graphics::SceneRenderData& scene,
            std::string& error) const;
    };
}