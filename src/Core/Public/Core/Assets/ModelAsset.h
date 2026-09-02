#pragma once

#include "Core/Assets/ModelResource.h"
#include "Core/Math/BoundingBox.h"

#include <optional>
#include <string>

namespace core::assets
{
    struct ModelAsset final
    {
        ModelResource resource;

        float extent = 0.0f;
        bool batched = false;

        std::optional<math::BoundingBox> visibilityBox;

        std::string visualReference;
        std::string visualLogicalPath;
        std::string primitivesLogicalPath;

        bool visualExists = false;
        bool primitivesExists = false;
    };
}