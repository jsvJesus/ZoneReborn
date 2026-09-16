#pragma once

#include "Core/Assets/TextureResource.h"

#include <cstddef>
#include <string>
#include <vector>

namespace core::assets
{
    struct TextureAnimation final
    {
        std::string logicalPath;

        float framesPerSecond =
            0.0f;

        std::vector<TextureResource>
            textures;

        std::vector<std::size_t>
            frameSequence;
    };
}