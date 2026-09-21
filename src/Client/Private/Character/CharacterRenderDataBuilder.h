#pragma once

#include "Character/CharacterCatalog.h"
#include "Character/CharacterModelComposer.h"
#include "Character/CharacterState.h"

#include "Graphics/SceneRenderData.h"

#include "Core/Math/Transform3x4.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <string>

namespace client::character
{
    class RenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const Catalog& catalog,
            const State& state,
            const core::math::Transform3x4& transform,
            graphics::SceneRenderData& scene,
            std::size_t& outputInstanceCount,
            std::string& error);
    };
}