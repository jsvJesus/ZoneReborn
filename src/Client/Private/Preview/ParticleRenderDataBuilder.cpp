#include "Preview/ParticleRenderDataBuilder.h"
#include "Preview/ParticleMeshRenderDataBuilder.h"

#include "Core/Assets/TextureAnimationLoader.h"
#include "Core/Images/DdsDecoder.h"
#include "Core/Log.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    struct AnimationRenderData final
    {
        float framesPerSecond =
            0.0f;

        std::vector<std::int32_t>
            frameTextureIndices;
    };

    bool IsMeshRenderer(
        const core::world::particles::ParticleRendererType type) noexcept
    {
        return
            type ==
                core::world::particles::ParticleRendererType::Mesh ||
            type ==
                core::world::particles::ParticleRendererType::Visual;
    }

    bool RequiresTexture(
        const core::world::particles::ParticleRendererType type) noexcept
    {
        using core::world::particles::ParticleRendererType;

        return
            type == ParticleRendererType::Sprite ||
            type == ParticleRendererType::SpriteBlend ||
            type == ParticleRendererType::Trail ||
            type == ParticleRendererType::PointSprite ||
            type == ParticleRendererType::Blur ||
            type == ParticleRendererType::Amp;
    }

    void DisableEmitter(
        client::graphics::SceneParticleEmitter& emitter,
        const std::string& reason)
    {
        emitter.renderable =
            false;

        core::Log::Warning(
            std::string(
                "Particle render dependency failed: ") +
            emitter.resource +
            "/" +
            emitter.system.name +
            ": " +
            reason);
    }
}

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
        std::size_t& outputAnimationCount,
        std::size_t& outputAnimationFrameCount,
        std::string& error) const
    {
        outputStaticEmitterCount =
            0;

        outputAnimatedEmitterCount =
            0;

        outputLoadedTextureCount =
            0;

        outputAnimationCount =
            0;

        outputAnimationFrameCount =
            0;

        error.clear();

        std::unordered_map<
            std::string,
            AnimationRenderData>
            animationCache;

        core::assets::TextureAnimationLoader
            animationLoader;

        ParticleMeshRenderDataBuilder
            meshBuilder;

        for (graphics::SceneParticleEmitter& emitter :
             scene.particleEmitters)
        {
            emitter.textureIndex =
                -1;

            emitter.animatedTexture =
                false;

            emitter.textureAnimationFps =
                0.0f;

            emitter.textureFrameIndices.clear();

            emitter.meshIndices.clear();

            emitter.materialFx =
                0;

            emitter.sortType =
                0;

            emitter.doubleSided =
                false;

            emitter.renderable =
                true;

            if (!emitter.system.hasRenderer)
            {
                continue;
            }

            const core::world::particles::ParticleRendererDefinition&
                renderer =
                    emitter.system.renderer;

            emitter.materialFx =
                renderer.materialFx;

            emitter.sortType =
                renderer.sortType;

            emitter.doubleSided =
                renderer.doubleSided;

            if (IsMeshRenderer(
                    renderer.type) &&
                !meshBuilder.Build(
                    resources,
                    renderer,
                    scene,
                    emitter.meshIndices,
                    error))
            {
                DisableEmitter(
                    emitter,
                    error);

                error.clear();

                continue;
            }

            const core::world::particles::ParticleTextureReference&
                texture =
                    renderer.texture;

            if (texture.sourceReference.empty() ||
                texture.logicalPath.empty())
            {
                if (RequiresTexture(
                        renderer.type))
                {
                    DisableEmitter(
                        emitter,
                        "renderer texture reference is empty.");
                }

                continue;
            }

            if (!texture.animated)
            {
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
                    DisableEmitter(
                        emitter,
                        error);

                    error.clear();

                    continue;
                }

                if (created)
                {
                    ++outputLoadedTextureCount;
                }

                ++outputStaticEmitterCount;

                continue;
            }

            emitter.animatedTexture =
                true;

            auto animationIterator =
                animationCache.find(
                    texture.logicalPath);

            if (animationIterator ==
                animationCache.end())
            {
                core::assets::TextureAnimation
                    animation;

                if (!animationLoader.Load(
                        resources,
                        texture.logicalPath,
                        animation,
                        error))
                {
                    DisableEmitter(
                        emitter,
                        error);

                    error.clear();

                    continue;
                }

                AnimationRenderData
                    renderData;

                renderData.framesPerSecond =
                    animation.framesPerSecond;

                std::vector<std::int32_t>
                    sourceTextureIndices;

                sourceTextureIndices.reserve(
                    animation.textures.size());

                bool animationFailed =
                    false;

                for (const core::assets::TextureResource& frameTexture :
                     animation.textures)
                {
                    std::int32_t textureIndex =
                        -1;

                    bool created =
                        false;

                    if (!ResolveTexture(
                            resources,
                            frameTexture.logicalPath,
                            scene,
                            textureIndex,
                            created,
                            error))
                    {
                        error =
                            animation.logicalPath +
                            ": " +
                            error;

                        animationFailed =
                            true;

                        break;
                    }

                    if (created)
                    {
                        ++outputLoadedTextureCount;
                    }

                    sourceTextureIndices.push_back(
                        textureIndex);
                }

                if (animationFailed)
                {
                    DisableEmitter(
                        emitter,
                        error);

                    error.clear();

                    continue;
                }

                renderData.frameTextureIndices.reserve(
                    animation.frameSequence.size());

                for (const std::size_t frameIndex :
                     animation.frameSequence)
                {
                    if (frameIndex >=
                        sourceTextureIndices.size())
                    {
                        error =
                            animation.logicalPath +
                            ": animation frame index is out of range.";

                        animationFailed =
                            true;

                        break;
                    }

                    renderData.frameTextureIndices.push_back(
                        sourceTextureIndices[
                            frameIndex]);
                }

                if (animationFailed)
                {
                    DisableEmitter(
                        emitter,
                        error);

                    error.clear();

                    continue;
                }

                if (renderData.frameTextureIndices.empty())
                {
                    error =
                        animation.logicalPath +
                        ": animation contains no renderable frames.";

                    DisableEmitter(
                        emitter,
                        error);

                    error.clear();

                    continue;
                }

                ++outputAnimationCount;

                outputAnimationFrameCount +=
                    renderData.frameTextureIndices.size();

                animationIterator =
                    animationCache.emplace(
                        texture.logicalPath,
                        std::move(
                            renderData)).first;
            }

            emitter.textureAnimationFps =
                animationIterator
                    ->second
                    .framesPerSecond;

            emitter.textureFrameIndices =
                animationIterator
                    ->second
                    .frameTextureIndices;

            if (emitter.textureFrameIndices.empty())
            {
                error =
                    "Particle animation contains no frame textures: " +
                    texture.logicalPath;

                DisableEmitter(
                    emitter,
                    error);

                error.clear();

                continue;
            }

            emitter.textureIndex =
                emitter.textureFrameIndices.front();

            ++outputAnimatedEmitterCount;
        }

        return true;
    }
}
