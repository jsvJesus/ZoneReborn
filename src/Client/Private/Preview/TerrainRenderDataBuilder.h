#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/TerrainLoader.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace client::preview
{
    class TerrainRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::world::TerrainAsset& terrain,
            graphics::SceneRenderData& scene,
            std::int32_t& outputMaterialIndex,
            std::string& error);

    private:
        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const core::world::TerrainLayerData& layer,
            graphics::SceneRenderData& scene,
            std::size_t& outputTextureIndex,
            std::string& error);

        std::unordered_map<
            std::string,
            std::size_t>
            textureCache_;
    };
}