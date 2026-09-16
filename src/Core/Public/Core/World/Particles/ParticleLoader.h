#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Particles/ParticleDefinition.h"

#include <string>
#include <string_view>

namespace core::world::particles
{
    class ParticleLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view resourceReference,
            ParticleDefinition& output,
            std::string& error) const;
    };
}