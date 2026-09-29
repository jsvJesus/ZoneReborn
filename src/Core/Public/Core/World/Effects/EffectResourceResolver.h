#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace core::resources
{
    class ResourceFileSystem;
}

namespace core::world::effects
{
    enum class EffectResourceKind : std::uint8_t
    {
        Particle = 0,
        Sfx
    };

    class EffectResourceResolver final
    {
    public:
        [[nodiscard]]
        static std::string Resolve(
            const resources::ResourceFileSystem& resources,
            std::string_view reference,
            EffectResourceKind kind);
    };
}
