#pragma once

#include "Core/Math/Transform3x4.h"
#include "Core/World/SpaceSettings.h"

#include <cstddef>
#include <string>
#include <vector>

namespace core::world
{
    struct WorldModelInstance final
    {
        std::string chunkId;
        std::string modelReference;

        math::Transform3x4 transform;

        bool shell = false;
    };

    struct WorldScene final
    {
        std::string spaceName;

        SpaceSettings settings;

        std::vector<WorldModelInstance>
            modelInstances;

        std::size_t chunkCount = 0;
        std::size_t outdoorChunkCount = 0;
        std::size_t indoorChunkCount = 0;

        std::size_t speedTreeInstanceCount = 0;
        std::size_t terrainReferenceCount = 0;
        std::size_t largeObjectReferenceCount = 0;
    };
}