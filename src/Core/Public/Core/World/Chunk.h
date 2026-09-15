#pragma once

#include "Core/Math/BoundingBox.h"
#include "Core/Math/Transform3x4.h"
#include "Core/Math/Vector3.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace core::world
{
    struct ChunkModelInstance final
    {
        std::string resource;

        math::Transform3x4 transform;

        bool reflectionVisible = false;
    };

    struct ChunkSpeedTreeInstance final
    {
        std::string resource;

        std::int32_t seed = 0;

        math::Transform3x4 transform;

        bool reflectionVisible = false;
    };

    struct ChunkTerrainReference final
    {
        std::string resource;
    };

    struct ChunkLargeObjectReference final
    {
        std::string uid;
        std::string type;
    };

    struct ChunkOmniLight final
    {
        std::string guid;

        math::Vector3 colour;
        math::Vector3 position;

        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        float multiplier = 1.0f;

        std::int32_t priority = 0;
        std::int32_t lightType = 0;

        bool isDynamic = false;
        bool isStatic = false;
        bool specular = false;
    };

    struct Chunk final
    {
        std::string spaceName;
        std::string chunkId;
        std::string resourcePath;

        std::optional<math::Transform3x4> transform;
        std::optional<math::BoundingBox> boundingBox;

        std::vector<ChunkModelInstance> models;
        std::vector<ChunkModelInstance> shells;

        std::vector<ChunkSpeedTreeInstance> speedTrees;

        std::vector<ChunkTerrainReference> terrains;

        std::vector<ChunkLargeObjectReference> largeObjects;

        std::vector<ChunkOmniLight> omniLights;

        std::vector<std::string> overlappers;

        [[nodiscard]]
        bool IsIndoor() const noexcept
        {
            return !shells.empty() ||
                   transform.has_value();
        }
    };
}