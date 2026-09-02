#pragma once

#include "Core/Assets/ModelResource.h"

#include <cstddef>
#include <vector>

namespace core::assets
{
    struct ModelSource final
    {
        ModelResource resource;

        std::vector<std::byte> data;
    };
}