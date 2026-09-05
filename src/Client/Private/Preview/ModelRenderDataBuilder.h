#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Assets/VisualAsset.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace client::preview
{
    class ModelRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::VisualGeometry& geometry,
            graphics::SceneRenderData& scene,
            graphics::SceneMesh& sceneMesh,
            std::size_t& outputTexturedGroups,
            std::string& error);

    private:
        [[nodiscard]]
        const core::assets::TextureResource*
        FindDiffuseTexture(
            const core::assets::VisualMaterial& material) const noexcept;

        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::TextureResource& texture,
            graphics::SceneRenderData& scene,
            std::size_t& outputTextureIndex,
            std::string& error);

        std::unordered_map<
            std::string,
            std::size_t>
            textureCache_;
    };
}