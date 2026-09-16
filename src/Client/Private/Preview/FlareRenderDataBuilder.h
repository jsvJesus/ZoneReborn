#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Assets/TextureResource.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Flare/FlareDefinition.h"
#include "Core/World/WorldScene.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace client::preview
{
    class FlareRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::world::WorldFlareInstance& source,
            graphics::SceneRenderData& scene,
            std::size_t& outputElementCount,
            std::string& error);

        [[nodiscard]]
        std::size_t LoadedDefinitionCount() const noexcept;

    private:
        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::TextureResource& texture,
            graphics::SceneRenderData& scene,
            std::size_t& outputTextureIndex,
            std::string& error);

        std::unordered_map<
            std::string,
            core::world::flare::FlareDefinition>
            definitions_;

        std::unordered_map<
            std::string,
            std::size_t>
            textureCache_;
    };
}