#pragma once

#include "Core/Assets/TextureResource.h"
#include "Core/Math/Vector3.h"

#include <string>
#include <vector>

namespace core::world::sky
{
    struct SkyColourKey final
    {
        float time = 0.0f;

        math::Vector3 colour;
    };

    struct SkyDefinition final
    {
        std::string resourcePath;

        assets::TextureResource
            gradientTexture;

        float mieAmount =
            0.1f;

        float turbidityOffset =
            0.0f;

        float turbidityFactor =
            0.0f;

        float vertexHeightEffect =
            0.0f;

        float sunHeightEffect =
            0.0f;

        float power =
            1.0f;

        float farPlane =
            600.0f;

        float sunAngleDegrees =
            80.0f;

        float moonAngleDegrees =
            60.0f;

        float hourLengthSeconds =
            160.0f;

        float startTimeHours =
            12.0f;

        std::vector<SkyColourKey>
            lightKeys;

        std::vector<SkyColourKey>
            ambientKeys;
    };
}