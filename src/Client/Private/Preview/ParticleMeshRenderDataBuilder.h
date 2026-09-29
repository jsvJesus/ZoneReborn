#pragma once

#include "Graphics/SceneRenderData.h"
#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Particles/ParticleRendererDefinition.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace client::preview
{
    class ParticleMeshRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const core::world::particles::ParticleRendererDefinition& renderer,
            graphics::SceneRenderData& scene,
            std::vector<std::size_t>& outputMeshIndices,
            std::string& error);

    private:
        std::unordered_map<
            std::string,
            std::vector<std::size_t>>
            visualCache_;

        ModelRenderDataBuilder
            materialBuilder_;
    };
}
