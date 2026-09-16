#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace client::preview
{
    class ParticleRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            graphics::SceneRenderData& scene,
            std::size_t& outputStaticEmitterCount,
            std::size_t& outputAnimatedEmitterCount,
            std::size_t& outputLoadedTextureCount,
            std::size_t& outputAnimationCount,
            std::size_t& outputAnimationFrameCount,
            std::string& error) const;

    private:
        [[nodiscard]]
        bool ResolveTexture(
            const core::resources::ResourceFileSystem& resources,
            const std::string& logicalPath,
            graphics::SceneRenderData& scene,
            std::int32_t& outputTextureIndex,
            bool& outputCreated,
            std::string& error) const;
    };
}