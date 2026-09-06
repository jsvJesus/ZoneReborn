#pragma once

#include "Core/Math/Transform3x4.h"

#include <string>

namespace core::world::flora
{
    struct FloraInstance final
    {
        std::string chunkId;
        std::string ecotypeName;
        std::string visualReference;

        math::Transform3x4
            transform;
    };
}