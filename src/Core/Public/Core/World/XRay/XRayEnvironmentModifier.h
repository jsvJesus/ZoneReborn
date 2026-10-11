#pragma once

#include "Core/Math/Vector3.h"
#include <cstdint>

namespace core::world::xray
{
    enum EnvironmentModifierFlags : std::uint16_t
    {
        ModifyViewDistance = 1u << 0u,
        ModifyFogColour = 1u << 1u,
        ModifyFogDensity = 1u << 2u,
        ModifyAmbient = 1u << 3u,
        ModifySkyColour = 1u << 4u,
        ModifyHemisphere = 1u << 5u,
        ModifyAllEnvironment = 0x3fu
    };

    struct EnvironmentModifier final
    {
        std::uint32_t chunkId = 0;
        math::Vector3 position{};
        float radius = 0.0f;
        float power = 0.0f;
        float farPlane = 0.0f;
        math::Vector3 fogColour{};
        float fogDensity = 0.0f;
        math::Vector3 ambient{};
        math::Vector3 skyColour{};
        math::Vector3 hemisphere{};
        std::uint16_t flags = ModifyAllEnvironment;
    };
}
