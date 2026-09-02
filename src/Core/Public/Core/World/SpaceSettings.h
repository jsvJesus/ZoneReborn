#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace core::world
{
    struct SpaceBounds final
    {
        std::int32_t minX = 0;
        std::int32_t maxX = 0;

        std::int32_t minY = 0;
        std::int32_t maxY = 0;

        [[nodiscard]]
        std::int32_t Width() const noexcept
        {
            return maxX - minX + 1;
        }

        [[nodiscard]]
        std::int32_t Height() const noexcept
        {
            return maxY - minY + 1;
        }

        [[nodiscard]]
        std::size_t OutdoorChunkCount() const noexcept
        {
            if (maxX < minX ||
                maxY < minY)
            {
                return 0;
            }

            return
                static_cast<std::size_t>(Width()) *
                static_cast<std::size_t>(Height());
        }
    };

    struct TerrainSettings final
    {
        std::int32_t version = 0;

        std::int32_t heightMapSize = 0;
        std::int32_t normalMapSize = 0;
        std::int32_t holeMapSize = 0;
        std::int32_t shadowMapSize = 0;
        std::int32_t blendMapSize = 0;
    };

    struct SpaceSettings final
    {
        std::string name;
        std::string resourcePath;

        SpaceBounds bounds;
        TerrainSettings terrain;

        std::string timeOfDay;
        std::string skyGradientDome;

        float farPlane = 0.0f;
    };
}