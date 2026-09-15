#include "Preview/WaterRenderDataBuilder.h"

#include "Core/World/Water/WaterMeshBuilder.h"

#include <cstdint>
#include <utility>

namespace client::preview
{
    bool WaterRenderDataBuilder::Build(
        const core::world::WorldLargeObjectReference& object,
        graphics::SceneRenderData& scene,
        WaterRenderData& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!object.vloLoaded)
        {
            error =
                "VLO resource is not loaded.";

            return false;
        }

        if (object.vloResource.type !=
            core::world::vlo::VloType::Water)
        {
            error =
                "VLO resource is not water.";

            return false;
        }

        if (!object.vloResource.water.has_value())
        {
            error =
                "Water VLO does not contain water definition.";

            return false;
        }

        const core::world::water::WaterDefinition&
            definition =
                *object.vloResource.water;

        core::world::water::WaterMeshBuilder
            meshBuilder;

        core::assets::MeshData
            mesh;

        core::math::Transform3x4
            transform;

        if (!meshBuilder.Build(
                definition,
                mesh,
                transform,
                error))
        {
            return false;
        }

        graphics::SceneWaterMaterial
            material;

        material.deepColour =
            definition.deepColour;

        const std::size_t materialIndex =
            scene.waterMaterials.size();

        scene.waterMaterials.push_back(
            material);

        graphics::SceneMesh
            sceneMesh;

        sceneMesh.geometry =
            std::move(
                mesh);

        sceneMesh.waterMaterialIndex =
            static_cast<std::int32_t>(
                materialIndex);

        const std::size_t vertexCount =
            sceneMesh.geometry.vertices.size();

        const std::size_t triangleCount =
            sceneMesh.geometry.TriangleCount();

        const std::size_t meshIndex =
            scene.meshes.size();

        scene.meshes.push_back(
            std::move(
                sceneMesh));

        graphics::SceneInstance
            instance;

        instance.meshIndex =
            meshIndex;

        instance.transform =
            transform;

        scene.instances.push_back(
            std::move(
                instance));

        output.meshIndex =
            meshIndex;

        output.materialIndex =
            materialIndex;

        output.vertexCount =
            vertexCount;

        output.triangleCount =
            triangleCount;

        return true;
    }
}