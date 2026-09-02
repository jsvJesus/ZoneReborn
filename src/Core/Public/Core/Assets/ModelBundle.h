#pragma once

#include "Core/Assets/ModelAsset.h"
#include "Core/Assets/PrimitivesContainer.h"
#include "Core/Assets/VisualAsset.h"

namespace core::assets
{
    struct ModelBundle final
    {
        ModelAsset model;
        VisualAsset visual;
        PrimitivesContainer primitives;
    };
}