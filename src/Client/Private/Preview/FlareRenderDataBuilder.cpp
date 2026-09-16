#include "Preview/FlareRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"
#include "Core/World/Flare/FlareLoader.h"

#include <algorithm>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace client::preview
{
    bool FlareRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::TextureResource& texture,
        graphics::SceneRenderData& scene,
        std::size_t& outputTextureIndex,
        std::string& error)
    {
        error.clear();

        if (!texture.exists ||
            texture.logicalPath.empty())
        {
            error =
                "Flare texture does not exist: " +
                texture.logicalPath;

            return false;
        }

        const auto cached =
            textureCache_.find(
                texture.logicalPath);

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
                texture.logicalPath,
                encoded))
        {
            error =
                "Unable to read flare texture: " +
                texture.logicalPath;

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
                texture.logicalPath +
                ": " +
                error;

            return false;
        }

        graphics::SceneTextureData
            sceneTexture;

        sceneTexture.logicalPath =
            texture.logicalPath;

        sceneTexture.image =
            std::move(
                image);

        sceneTexture.generateMipmaps =
            false;

        outputTextureIndex =
            scene.textures.size();

        scene.textures.push_back(
            std::move(
                sceneTexture));

        textureCache_.emplace(
            texture.logicalPath,
            outputTextureIndex);

        return true;
    }

    bool FlareRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::WorldFlareInstance& source,
        graphics::SceneRenderData& scene,
        std::size_t& outputElementCount,
        std::string& error)
    {
        outputElementCount =
            0;

        error.clear();

        auto definitionIterator =
            definitions_.find(
                source.resource);

        if (definitionIterator ==
            definitions_.end())
        {
            core::world::flare::FlareLoader
                loader;

            core::world::flare::FlareDefinition
                definition;

            if (!loader.Load(
                    resources,
                    source.resource,
                    definition,
                    error))
            {
                return false;
            }

            definitionIterator =
                definitions_.emplace(
                    source.resource,
                    std::move(
                        definition)).first;
        }

        const core::world::flare::FlareDefinition&
            definition =
                definitionIterator->second;

        std::string
            lastTextureError;

        for (const core::world::flare::FlareElementDefinition& element :
             definition.elements)
        {
            std::size_t
                textureIndex =
                    0;

            std::string
                textureError;

            if (!ResolveTexture(
                    resources,
                    element.texture,
                    scene,
                    textureIndex,
                    textureError))
            {
                lastTextureError =
                    std::move(
                        textureError);

                continue;
            }

            graphics::SceneFlare
                flare;

            flare.guid =
                source.guid;

            flare.resource =
                source.resource;

            flare.position =
            {
                source.position.x,
                source.position.y,
                source.position.z
            };

            flare.colour =
            {
                std::clamp(
                    source.colour.x /
                        255.0f *
                    element.rgba[0] /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.y /
                        255.0f *
                    element.rgba[1] /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.z /
                        255.0f *
                    element.rgba[2] /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    element.rgba[3] /
                        255.0f,
                    0.0f,
                    1.0f)
            };

            flare.textureIndex =
                textureIndex;

            flare.size =
                std::max(
                    element.size,
                    0.0f);

            flare.depth =
                element.depth;

            flare.maxDistance =
                std::max(
                    source.maxDistance,
                    0.0f);

            flare.area =
                std::max(
                    source.area,
                    0.0f);

            flare.fadeSpeed =
                std::max(
                    source.fadeSpeed,
                    0.001f);

            scene.flares.push_back(
                std::move(
                    flare));

            ++outputElementCount;
        }

        if (outputElementCount ==
            0)
        {
            if (!lastTextureError.empty())
            {
                error =
                    lastTextureError;
            }
            else
            {
                error =
                    "Flare produced no renderable elements.";
            }

            return false;
        }

        return true;
    }

    std::size_t
    FlareRenderDataBuilder::LoadedDefinitionCount() const noexcept
    {
        return
            definitions_.size();
    }
}