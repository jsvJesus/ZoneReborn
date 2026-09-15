#pragma once

#include "Core/Assets/TextureResource.h"
#include "Core/Math/Vector3.h"

#include <array>
#include <cstdint>

namespace core::world::water
{
    struct WaterDefinition final
    {
        math::Vector3 position;
        float orientation = 0.0f;
        math::Vector3 size;

        float fresnelConstant = 0.0f;
        float fresnelExponent = 0.0f;

        std::array<float, 4> reflectionTint{};
        float reflectionStrength = 0.0f;

        std::array<float, 4> refractionTint{};
        float refractionStrength = 0.0f;

        float tessellation = 0.0f;
        float consistency = 0.0f;
        float textureTessellation = 0.0f;

        std::array<float, 2> scrollSpeed1{};
        std::array<float, 2> scrollSpeed2{};
        std::array<float, 2> waveScale{};

        float windVelocity = 0.0f;

        float sunPower = 0.0f;
        float sunScale = 0.0f;

        assets::TextureResource waveTexture;

        float cellSize = 0.0f;
        float smoothness = 0.0f;
        float depth = 0.0f;

        assets::TextureResource foamTexture;
        assets::TextureResource reflectionTexture;

        std::array<float, 4> deepColour{};

        float fadeDepth = 0.0f;

        float foamIntersection = 0.0f;
        float foamMultiplier = 0.0f;
        float foamTiling = 0.0f;

        bool bypassDepth = false;
        bool useCubeMap = false;
        bool useEdgeAlpha = false;
        bool useSimulation = false;

        std::int32_t visibility = 0;
        std::int32_t depthStr = 0;

        float physicDepth = 0.0f;
    };
}