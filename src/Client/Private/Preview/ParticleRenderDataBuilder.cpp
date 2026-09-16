#include "Preview/ParticleRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace client::preview
{
    bool ParticleRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const std::string& logicalPath,
        graphics::SceneRenderData& scene,
        std::int32_t& outputTextureIndex,
        bool& outputCreated,
        std::string& error) const
    {
        outputTextureIndex =
            -1;

        outputCreated =
            false;

        error.clear();

        if (logicalPath.empty())
        {
            error =
                "Particle renderer contains empty texture path.";

            return false;
        }

        for (std::size_t index = 0;
             index < scene.textures.size();
             ++index)
        {
            if (scene.textures[index].logicalPath !=
                logicalPath)
            {
                continue;
            }

            if (index >
                static_cast<std::size_t>(
                    std::numeric_limits<std::int32_t>::max()))
            {
                error =
                    "Particle texture index exceeds int32 range.";

                return false;
            }

            outputTextureIndex =
                static_cast<std::int32_t>(
                    index);

            return true;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "Particle texture does not exist: " +
                logicalPath;

            return false;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                logicalPath,
                encoded))
        {
            error =
                "Unable to read particle texture: " +
                logicalPath;

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
                logicalPath +
                ": " +
                error;

            return false;
        }

        if (scene.textures.size() >
            static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max()))
        {
            error =
                "Particle texture index exceeds int32 range.";

            return false;
        }

        graphics::SceneTextureData
            texture;

        texture.logicalPath =
            logicalPath;

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

        outputCreated =
            true;

        return true;
    }

    bool ParticleRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        graphics::SceneRenderData& scene,
        std::size_t& outputStaticEmitterCount,
        std::size_t& outputAnimatedEmitterCount,
        std::size_t& outputLoadedTextureCount,
        std::string& error) const
    {
        outputStaticEmitterCount =
            0;

        outputAnimatedEmitterCount =
            0;

        outputLoadedTextureCount =
            0;

        error.clear();

        for (graphics::SceneParticleEmitter& emitter :
             scene.particleEmitters)
        {
            emitter.textureIndex =
                -1;

            emitter.animatedTexture =
                false;

            if (!emitter.system.hasRenderer)
            {
                continue;
            }

            const core::world::particles::ParticleTextureReference&
                texture =
                    emitter.system.renderer.texture;

            if (texture.sourceReference.empty() ||
                texture.logicalPath.empty())
            {
                continue;
            }

            if (texture.animated)
            {
                emitter.animatedTexture =
                    true;

                ++outputAnimatedEmitterCount;

                continue;
            }

            bool created =
                false;

            if (!ResolveTexture(
                    resources,
                    texture.logicalPath,
                    scene,
                    emitter.textureIndex,
                    created,
                    error))
            {
                error =
                    emitter.resource +
                    "/" +
                    emitter.system.name +
                    ": " +
                    error;

                return false;
            }

            if (created)
            {
                ++outputLoadedTextureCount;
            }

            ++outputStaticEmitterCount;
        }

        return true;
    }
}