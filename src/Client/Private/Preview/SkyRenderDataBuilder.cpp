#include "Preview/SkyRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"

#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace client::preview
{
    bool SkyRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::sky::SkyDefinition& definition,
        graphics::SceneRenderData& scene,
        std::string& error) const
    {
        error.clear();

        scene.sky = {};

        scene.sky.enabled =
            true;

        scene.sky.definition =
            definition;

        scene.sky.gradientTextureIndex =
            -1;

        const core::assets::TextureResource& texture =
            definition.gradientTexture;

        if (!texture.exists ||
            texture.logicalPath.empty())
        {
            return true;
        }

        for (std::size_t index = 0;
             index <
                scene.textures.size();
             ++index)
        {
            if (scene.textures[index].logicalPath !=
                texture.logicalPath)
            {
                continue;
            }

            if (index >
                static_cast<std::size_t>(
                    std::numeric_limits<
                        std::int32_t>::max()))
            {
                error =
                    "Sky texture index overflow.";

                return false;
            }

            scene.sky.gradientTextureIndex =
                static_cast<std::int32_t>(
                    index);

            return true;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                texture.logicalPath,
                encoded))
        {
            error =
                "Unable to read sky gradient DDS: " +
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

        if (scene.textures.size() >
            static_cast<std::size_t>(
                std::numeric_limits<
                    std::int32_t>::max()))
        {
            error =
                "Sky texture index overflow.";

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

        scene.sky.gradientTextureIndex =
            static_cast<std::int32_t>(
                scene.textures.size());

        scene.textures.push_back(
            std::move(
                sceneTexture));

        return true;
    }
}