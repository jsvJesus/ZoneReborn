#include "Preview/FloraRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace client::preview
{
    bool FloraRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::flora::FloraVisualAsset& asset,
        const float alphaCutoff,
        graphics::SceneRenderData& scene,
        FloraRenderData& output,
        std::string& error)
    {
        output = {};
        error.clear();

        const float resolvedAlphaCutoff =
            std::clamp(
                alphaCutoff,
                0.0f,
                1.0f);

        core::assets::MeshLoader
            meshLoader;

        FloraRenderData
            result;

        for (const core::assets::VisualRenderSet& renderSet :
             asset.visual.renderSets)
        {
            for (const core::assets::VisualGeometry& geometry :
                 renderSet.geometries)
            {
                core::assets::MeshData
                    mesh;

                std::string meshError;

                if (!meshLoader.Load(
                        asset.primitives,
                        geometry,
                        mesh,
                        meshError))
                {
                    error =
                        asset.visualLogicalPath +
                        ": " +
                        meshError;

                    return false;
                }

                graphics::SceneMesh
                    sceneMesh;

                sceneMesh.geometry =
                    std::move(
                        mesh);

                std::size_t texturedGroups =
                    0;

                std::string materialError;

                if (!materialBuilder_.Build(
                        resources,
                        geometry,
                        scene,
                        sceneMesh,
                        texturedGroups,
                        materialError))
                {
                    error =
                        asset.visualLogicalPath +
                        ": " +
                        materialError;

                    return false;
                }

                for (graphics::SceneModelMaterial& material :
                     sceneMesh.modelMaterials)
                {
                    if (material.diffuseTextureIndex <
                        0)
                    {
                        continue;
                    }

                    material.alphaMode =
                        graphics::SceneAlphaMode::Cutout;

                    material.alphaCutoff =
                        resolvedAlphaCutoff;
                }

                result.triangleCount +=
                    sceneMesh.geometry.TriangleCount();

                result.texturedPrimitiveGroups +=
                    texturedGroups;

                const std::size_t meshIndex =
                    scene.meshes.size();

                scene.meshes.push_back(
                    std::move(
                        sceneMesh));

                result.meshIndices.push_back(
                    meshIndex);
            }
        }

        if (result.meshIndices.empty())
        {
            error =
                "SO flora visual contains no renderable geometry: " +
                asset.visualLogicalPath;

            return false;
        }

        output =
            std::move(
                result);

        return true;
    }
}