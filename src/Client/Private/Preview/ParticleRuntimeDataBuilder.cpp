#include "Preview/ParticleRuntimeDataBuilder.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace client::preview
{
    bool ParticleRuntimeDataBuilder::Build(
        const core::world::WorldScene& world,
        const std::unordered_map<
            std::string,
            core::world::particles::ParticleDefinition>& definitions,
        graphics::SceneRenderData& scene,
        std::size_t& outputEmitterCount,
        std::size_t& outputCapacity,
        std::string& error) const
    {
        outputEmitterCount =
            0;

        outputCapacity =
            0;

        error.clear();

        scene.particleEmitters.clear();

        for (const core::world::WorldParticleInstance& instance :
             world.particleInstances)
        {
            const auto definitionIterator =
                definitions.find(
                    instance.particleLogicalPath);

            if (definitionIterator ==
                definitions.end())
            {
                error =
                    "Particle runtime definition not loaded: " +
                    instance.particleLogicalPath;

                return false;
            }

            const core::world::particles::ParticleDefinition& definition =
                definitionIterator->second;

            for (const core::world::particles::ParticleSystemDefinition& system :
                 definition.systems)
            {
                graphics::SceneParticleEmitter
                    emitter;

                emitter.resource =
                    instance.particleLogicalPath;

                emitter.system =
                    system;

                emitter.transform =
                    instance.transform;

                outputCapacity +=
                    system.capacity >
                        0
                        ? static_cast<std::size_t>(
                            system.capacity)
                        : 0;

                scene.particleEmitters.push_back(
                    std::move(
                        emitter));

                ++outputEmitterCount;
            }
        }

        return true;
    }
}