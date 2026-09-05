#include "Preview/TerrainRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::uint8_t SampleBlend(
        const core::world::TerrainLayerData& layer,
        const std::uint32_t targetX,
        const std::uint32_t targetZ,
        const std::uint32_t targetWidth,
        const std::uint32_t targetHeight) noexcept
    {
        if (layer.width == 0 ||
            layer.height == 0 ||
            targetWidth == 0 ||
            targetHeight == 0)
        {
            return 0;
        }

        const float normalizedX =
            targetWidth > 1
                ? static_cast<float>(
                    targetX) /
                    static_cast<float>(
                        targetWidth - 1)
                : 0.0f;

        const float normalizedZ =
            targetHeight > 1
                ? static_cast<float>(
                    targetZ) /
                    static_cast<float>(
                        targetHeight - 1)
                : 0.0f;

        const std::uint32_t sourceX =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedX *
                    static_cast<float>(
                        layer.width - 1) +
                    0.5f),
                layer.width - 1);

        const std::uint32_t sourceZ =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedZ *
                    static_cast<float>(
                        layer.height - 1) +
                    0.5f),
                layer.height - 1);

        return layer.BlendAt(
            sourceX,
            sourceZ);
    }
}

namespace client::preview
{
    bool TerrainRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const core::world::TerrainLayerData& layer,
        graphics::SceneRenderData& scene,
        std::size_t& outputTextureIndex,
        std::string& error)
    {
        outputTextureIndex = 0;

        if (!layer.texture.exists)
        {
            error =
                "Terrain texture was not found: " +
                layer.texture.logicalPath;

            return false;
        }

        const auto cached =
            textureCache_.find(
                layer.texture.logicalPath);

        if (cached !=
            textureCache_.end())
        {
            outputTextureIndex =
                cached->second;

            return true;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                layer.texture.logicalPath,
                encoded))
        {
            error =
                "Unable to read terrain DDS: " +
                layer.texture.logicalPath;

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
                layer.texture.logicalPath +
                ": " +
                error;

            return false;
        }

        graphics::SceneTextureData
            texture;

        texture.logicalPath =
            layer.texture.logicalPath;

        texture.image =
            std::move(image);

        const std::size_t textureIndex =
            scene.textures.size();

        scene.textures.push_back(
            std::move(texture));

        textureCache_.emplace(
            layer.texture.logicalPath,
            textureIndex);

        outputTextureIndex =
            textureIndex;

        return true;
    }

    bool TerrainRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::TerrainAsset& terrain,
        graphics::SceneRenderData& scene,
        std::int32_t& outputMaterialIndex,
        std::string& error)
    {
        outputMaterialIndex =
            -1;

        error.clear();

        if (terrain.layers.empty())
        {
            if (terrain.auxiliary.lodTextureDds.empty())
            {
                return true;
            }

            const std::string textureKey =
                terrain.cdataLogicalPath +
                "#terrain2/lodTexture.dds";

            std::size_t textureIndex =
                0;

            const auto cached =
                textureCache_.find(
                    textureKey);

            if (cached !=
                textureCache_.end())
            {
                textureIndex =
                    cached->second;
            }
            else
            {
                core::images::DdsDecoder
                    decoder;

                core::images::RgbaImage
                    image;

                if (!decoder.Decode(
                        std::span<const std::byte>(
                            terrain.auxiliary.lodTextureDds.data(),
                            terrain.auxiliary.lodTextureDds.size()),
                        image,
                        error))
                {
                    error =
                        "Unable to decode embedded terrain LOD texture: " +
                        error;

                    return false;
                }

                graphics::SceneTextureData
                    texture;

                texture.logicalPath =
                    textureKey;

                texture.image =
                    std::move(
                        image);

                textureIndex =
                    scene.textures.size();

                scene.textures.push_back(
                    std::move(
                        texture));

                textureCache_.emplace(
                    textureKey,
                    textureIndex);
            }

            graphics::SceneTerrainMaterial
                material;

            graphics::SceneTerrainPass
                pass;

            pass.layerCount =
                1;

            pass.layers[0].textureIndex =
                textureIndex;

            pass.layers[0].uProjection =
            {
                0.01f,
                0.0f,
                0.0f,
                0.0f
            };

            pass.layers[0].vProjection =
            {
                0.0f,
                0.0f,
                0.01f,
                0.0f
            };

            pass.blendMap.width =
                1;

            pass.blendMap.height =
                1;

            pass.blendMap.pixels =
            {
                std::byte{255},
                std::byte{0},
                std::byte{0},
                std::byte{0}
            };

            material.passes.push_back(
                std::move(
                    pass));

            outputMaterialIndex =
                static_cast<std::int32_t>(
                    scene.terrainMaterials.size());

            scene.terrainMaterials.push_back(
                std::move(
                    material));

            return true;
        }

        graphics::SceneTerrainMaterial
            material;

        constexpr std::size_t LayersPerPass =
            4;

        for (std::size_t firstLayer = 0;
             firstLayer <
                terrain.layers.size();
             firstLayer +=
                LayersPerPass)
        {
            const std::size_t layerCount =
                std::min(
                    LayersPerPass,
                    terrain.layers.size() -
                        firstLayer);

            std::uint32_t blendWidth = 0;
            std::uint32_t blendHeight = 0;

            for (std::size_t index = 0;
                 index < layerCount;
                 ++index)
            {
                const core::world::TerrainLayerData& layer =
                    terrain.layers[
                        firstLayer +
                        index];

                blendWidth =
                    std::max(
                        blendWidth,
                        layer.width);

                blendHeight =
                    std::max(
                        blendHeight,
                        layer.height);
            }

            if (blendWidth == 0 ||
                blendHeight == 0)
            {
                error =
                    "Terrain material contains invalid blend dimensions.";

                return false;
            }

            graphics::SceneTerrainPass pass;

            pass.layerCount =
                static_cast<std::uint32_t>(
                    layerCount);

            pass.blendMap.width =
                blendWidth;

            pass.blendMap.height =
                blendHeight;

            pass.blendMap.pixels.assign(
                static_cast<std::size_t>(
                    blendWidth) *
                    blendHeight *
                    4,
                std::byte{0});

            for (std::size_t localLayer = 0;
                 localLayer < layerCount;
                 ++localLayer)
            {
                const core::world::TerrainLayerData& layer =
                    terrain.layers[
                        firstLayer +
                        localLayer];

                std::size_t textureIndex = 0;

                if (!ResolveTexture(
                        resources,
                        layer,
                        scene,
                        textureIndex,
                        error))
                {
                    return false;
                }

                pass.layers[
                    localLayer].textureIndex =
                        textureIndex;

                pass.layers[
                    localLayer].uProjection =
                        layer.uProjection;

                pass.layers[
                    localLayer].vProjection =
                        layer.vProjection;

                for (std::uint32_t z = 0;
                     z < blendHeight;
                     ++z)
                {
                    for (std::uint32_t x = 0;
                         x < blendWidth;
                         ++x)
                    {
                        const std::size_t pixel =
                            (
                                static_cast<std::size_t>(z) *
                                    blendWidth +
                                x
                            ) *
                            4;

                        pass.blendMap.pixels[
                            pixel +
                            localLayer] =
                            static_cast<std::byte>(
                                SampleBlend(
                                    layer,
                                    x,
                                    z,
                                    blendWidth,
                                    blendHeight));
                    }
                }
            }

            material.passes.push_back(
                std::move(pass));
        }

        if (scene.terrainMaterials.size() >
            static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max()))
        {
            error =
                "Too many terrain materials.";

            return false;
        }

        outputMaterialIndex =
            static_cast<std::int32_t>(
                scene.terrainMaterials.size());

        scene.terrainMaterials.push_back(
            std::move(material));

        return true;
    }
}