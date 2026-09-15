#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Assets/TextureResource.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/WorldScene.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

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
            const core::resources::ResourceFileSystem& resources,
            const core::world::WorldLargeObjectReference& object,
            graphics::SceneRenderData& scene,
            WaterRenderData& output,
            std::string& error);

    private:
        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::TextureResource& resource,
            graphics::SceneRenderData& scene,
            std::int32_t& outputTextureIndex,
            std::string& error);

        std::unordered_map<
            std::string,
            std::int32_t>
            textureCache_;
    };
}