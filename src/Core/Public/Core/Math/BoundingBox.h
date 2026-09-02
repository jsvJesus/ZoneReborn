#pragma once

#include "Core/Math/Vector3.h"

namespace core::math
{
    struct BoundingBox final
    {
        Vector3 minimum;
        Vector3 maximum;
    };
}