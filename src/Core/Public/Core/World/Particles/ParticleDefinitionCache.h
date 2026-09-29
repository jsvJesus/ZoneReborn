#pragma once

#include "Core/World/Particles/ParticleDefinition.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace core::resources
{
    class ResourceFileSystem;
}

namespace core::world::particles
{
    class ParticleDefinitionCache final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view resourceReference,
            const ParticleDefinition*& output,
            std::string& error);

        [[nodiscard]]
        const std::unordered_map<std::string, ParticleDefinition>&
        Definitions() const noexcept;

        void Clear() noexcept;

        [[nodiscard]]
        std::size_t Size() const noexcept;

    private:
        std::unordered_map<std::string, ParticleDefinition>
            definitions_;
    };
}
