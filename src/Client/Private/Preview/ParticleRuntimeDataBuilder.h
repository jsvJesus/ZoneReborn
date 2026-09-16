#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/World/Particles/ParticleDefinition.h"
#include "Core/World/WorldScene.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace client::preview
{
    class ParticleRuntimeDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::world::WorldScene& world,
            const std::unordered_map<
                std::string,
                core::world::particles::ParticleDefinition>& definitions,
            graphics::SceneRenderData& scene,
            std::size_t& outputEmitterCount,
            std::size_t& outputCapacity,
            std::string& error) const;
    };
}