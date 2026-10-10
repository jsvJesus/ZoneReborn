#pragma once

#include "Core/Math/Vector3.h"
#include <cstdint>

namespace core::world::xray
{
    struct LevelLight final
    {
        std::uint32_t controller = 0;
        std::uint32_t type = 0; // Native Flight: point=1, spot=2, directional=3.
        math::Vector3 colour{};
        math::Vector3 position{};
        math::Vector3 direction{};
        float range = 0;
        float coneRadians = 0;
    };

    struct SpawnLamp final
    {
        std::uint32_t colour = 0xffffffffu;
        std::uint16_t flags = 0;
        float brightness = 0;
        float range = 0;
        float coneRadians = 0;
        float health = 0;
    };
}
