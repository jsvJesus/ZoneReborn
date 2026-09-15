#include "Preview/WaterRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"
#include "Core/World/Water/WaterMeshBuilder.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace client::preview
{
    bool WaterRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::TextureResource& resource,
        graphics::SceneRenderData& scene,
        std::int32_t& outputTextureIndex,
        std::string& error)
    {
        outputTextureIndex =
            -1;

        error.clear();

        if (!resource.exists ||
            resource.logicalPath.empty())
        {
            return true;
        }

        const auto cached =
            textureCache_.find(
                resource.logicalPath);

        if (cached !=
            textureCache_.end())
        {
            outputTextureIndex =
                cached->second;

            return true;
        }

        for (std::size_t index = 0;
             index < scene.textures.size();
             ++index)
        {
            if (scene.textures[index].logicalPath ==
                resource.logicalPath)
            {
                if (index >
                    static_cast<std::size_t>(
                        std::numeric_limits<
                            std::int32_t>::max()))
                {
                    error =
                        "Water texture index overflow.";

                    return false;
                }

                outputTextureIndex =
                    static_cast<std::int32_t>(
                        index);

                textureCache_.emplace(
                    resource.logicalPath,
                    outputTextureIndex);

                return true;
            }
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                resource.logicalPath,
                encoded))
        {
            error =
                "Unable to read water DDS: " +
                resource.logicalPath;

            return false;
        }

        core::images::DdsDecoder
            decoder;

        core::images::RgbaImage
            image;

        if (!decoder.Decode(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                image,
                error))
        {
            error =
                resource.logicalPath +
                ": " +
                error;

            return false;
        }

        if (scene.textures.size() >
            static_cast<std::size_t>(
                std::numeric_limits<
                    std::int32_t>::max()))
        {
            error =
                "Too many scene textures.";

            return false;
        }

        graphics::SceneTextureData
            texture;

        texture.logicalPath =
            resource.logicalPath;

        texture.image =
            std::move(
                image);

        texture.generateMipmaps =
            true;

        outputTextureIndex =
            static_cast<std::int32_t>(
                scene.textures.size());

        scene.textures.push_back(
            std::move(
                texture));

        textureCache_.emplace(
            resource.logicalPath,
            outputTextureIndex);

        return true;
    }

    bool WaterRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::WorldLargeObjectReference& object,
        graphics::SceneRenderData& scene,
        WaterRenderData& output,
        std::string& error)
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

        if (object.chunkIds.empty())
        {
            error =
                "Water VLO is not referenced by any chunk.";

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

        if (!ResolveTexture(
                resources,
                definition.waveTexture,
                scene,
                material.waveTextureIndex,
                error))
        {
            return false;
        }

        if (!ResolveTexture(
                resources,
                definition.foamTexture,
                scene,
                material.foamTextureIndex,
                error))
        {
            return false;
        }

        material.deepColour =
            definition.deepColour;

        material.reflectionTint =
            definition.reflectionTint;

        material.refractionTint =
            definition.refractionTint;

        material.waveScale =
            definition.waveScale;

        material.scrollSpeed1 =
            definition.scrollSpeed1;

        material.scrollSpeed2 =
            definition.scrollSpeed2;

        material.reflectionStrength =
            definition.reflectionStrength;

        material.refractionStrength =
            definition.refractionStrength;

        material.fresnelConstant =
            definition.fresnelConstant;

        material.fresnelExponent =
            definition.fresnelExponent;

        material.windVelocity =
            definition.windVelocity;

        material.textureTessellation =
            definition.textureTessellation;

        material.foamIntersection =
            definition.foamIntersection;

        material.foamMultiplier =
            definition.foamMultiplier;

        material.foamTiling =
            definition.foamTiling;

        material.depth =
            definition.depth;

        material.fadeDepth =
            definition.fadeDepth;

        material.smoothness =
            definition.smoothness;

        material.sunPower =
            definition.sunPower;

        material.sunScale =
            definition.sunScale;

        material.useEdgeAlpha =
            definition.useEdgeAlpha;

        material.useSimulation =
            definition.useSimulation;

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