#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Assets/SpeedTree/CTreeData.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace client::preview
{
    struct SpeedTreeLodRenderData final
    {
        std::vector<std::size_t>
            meshIndices;

        std::size_t triangleCount =
            0;
    };

    struct SpeedTreeRenderData final
    {
        std::array<
            SpeedTreeLodRenderData,
            3>
            lods{};

        std::array<float, 3>
            maximumDistances{};

        std::vector<std::size_t>
            fixedMeshIndices;

        std::size_t sourceLodCount =
            0;

        bool usesLodChain =
            false;

        bool hasBillboard =
            false;

        std::size_t billboardMeshIndex =
            std::numeric_limits<std::size_t>::max();

        std::size_t billboardTriangles =
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
            std::size_t requestedLod,
            bool cutout,
            graphics::SceneRenderData& scene,
            std::size_t& outputMeshIndex,
            std::size_t& outputTriangleCount,
            std::string& error);

        [[nodiscard]]
        bool BuildLeaves(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::speedtree::CTreeLeafGeometry& source,
            std::size_t requestedLod,
            graphics::SceneRenderData& scene,
            std::size_t& outputMeshIndex,
            std::size_t& outputTriangleCount,
            std::string& error);

        [[nodiscard]]
        bool BuildBillboard(
            const core::resources::ResourceFileSystem& resources,
            const core::assets::speedtree::CTreeBillboardGeometry& source,
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