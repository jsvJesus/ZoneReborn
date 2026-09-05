#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Assets/SpeedTree/CTreeData.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace client::preview
{
    struct SpeedTreeRenderData final
    {
        std::vector<std::size_t>
            meshIndices;

        std::size_t branchTriangles =
            0;

        std::size_t frondTriangles =
            0;
    };

    class SpeedTreeRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::speedtree::CTreeAsset& tree,
            graphics::SceneRenderData& scene,
            SpeedTreeRenderData& output,
            std::string& error);

    private:
        [[nodiscard]]
        bool BuildIndexedGeometry(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::speedtree::CTreeIndexedGeometry& source,
            bool cutout,
            graphics::SceneRenderData& scene,
            std::size_t& outputMeshIndex,
            std::size_t& outputTriangleCount,
            std::string& error);

        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const std::string& logicalPath,
            graphics::SceneRenderData& scene,
            std::size_t& outputTextureIndex,
            std::string& error);

        std::unordered_map<
            std::string,
            std::size_t>
            textureCache_;
    };
}