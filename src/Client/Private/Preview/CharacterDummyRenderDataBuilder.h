#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Math/Transform3x4.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>

namespace client::preview
{
    class CharacterDummyRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool BuildDefault(
            const core::resources::ResourceFileSystem& resources,
            const core::math::Transform3x4& transform,
            graphics::SceneRenderData& scene,
            std::string& error);
    };
}