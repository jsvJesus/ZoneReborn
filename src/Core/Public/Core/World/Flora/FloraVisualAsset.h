#pragma once

#include "Core/Assets/PrimitivesContainer.h"
#include "Core/Assets/VisualAsset.h"

#include <string>

namespace core::world::flora
{
    struct FloraVisualAsset final
    {
        std::string visualReference;

        std::string visualLogicalPath;
        std::string primitivesLogicalPath;

        assets::VisualAsset
            visual;

        assets::PrimitivesContainer
            primitives;
    };
}