#include "Core/World/Particles/ParticleDefinitionCache.h"

#include "Core/World/Effects/EffectResourceResolver.h"
#include "Core/World/Particles/ParticleLoader.h"

#include <string>
#include <string_view>
#include <utility>

namespace core::world::particles
{
    bool ParticleDefinitionCache::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view resourceReference,
        const ParticleDefinition*& output,
        std::string& error)
    {
        output =
            nullptr;

        error.clear();

        const std::string logicalPath =
            core::world::effects::EffectResourceResolver::Resolve(
                resources,
                resourceReference,
                core::world::effects::EffectResourceKind::Particle);

        if (logicalPath.empty())
        {
            error =
                "Particle resource reference is invalid or missing: " +
                std::string(
                    resourceReference);

            return false;
        }

        const auto cached =
            definitions_.find(
                logicalPath);

        if (cached !=
            definitions_.end())
        {
            output =
                &cached->second;

            return true;
        }

        ParticleDefinition
            definition;

        ParticleLoader
            loader;

        if (!loader.Load(
                resources,
                logicalPath,
                definition,
                error))
        {
            return false;
        }

        auto [iterator, inserted] =
            definitions_.emplace(
                logicalPath,
                std::move(
                    definition));

        static_cast<void>(
            inserted);

        output =
            &iterator->second;

        return true;
    }

    const std::unordered_map<std::string, ParticleDefinition>&
    ParticleDefinitionCache::Definitions() const noexcept
    {
        return
            definitions_;
    }

    void ParticleDefinitionCache::Clear() noexcept
    {
        definitions_.clear();
    }

    std::size_t ParticleDefinitionCache::Size() const noexcept
    {
        return
            definitions_.size();
    }
}
