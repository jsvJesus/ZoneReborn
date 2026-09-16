#pragma once

#include "Core/Assets/TextureResource.h"

#include <array>
#include <string>
#include <vector>

namespace core::world::flare
{
    struct FlareElementDefinition final
    {
        std::string materialReference;

        core::assets::TextureResource
            texture;

        float size =
            1.0f;

        float depth =
            1.0f;

        std::array<float, 4>
            rgba
        {
            255.0f,
            255.0f,
            255.0f,
            255.0f
        };
    };

    struct FlareDefinition final
    {
        std::string logicalPath;

        std::vector<FlareElementDefinition>
            elements;
    };
}