#include "WorldEditor/Terrain/TerrainData.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace world_editor::terrain {

    bool Heightfield::valid() const noexcept {
        return samplesX >= 2 &&
               samplesZ >= 2 &&
               std::isfinite(spacingMeters) &&
               spacingMeters > 0.0f &&
               std::isfinite(originX) &&
               std::isfinite(originZ) &&
               heights.size() ==
                   static_cast<std::size_t>(samplesX) * samplesZ;
    }

    float Heightfield::height_at(
        const std::uint32_t x,
        const std::uint32_t z
    ) const {
        if (!valid() || x >= samplesX || z >= samplesZ) {
            throw std::out_of_range(
                "Terrain sample is out of range"
            );
        }

        return heights[
            static_cast<std::size_t>(z) * samplesX + x
        ];
    }

    float Heightfield::sample_height(
        const float worldX,
        const float worldZ
    ) const {
        if (!valid() ||
            !std::isfinite(worldX) ||
            !std::isfinite(worldZ)) {
            throw std::invalid_argument(
                "Invalid terrain or world coordinates"
            );
        }

        const float gridX = std::clamp(
            (worldX - originX) / spacingMeters,
            0.0f,
            static_cast<float>(samplesX - 1)
        );

        const float gridZ = std::clamp(
            (worldZ - originZ) / spacingMeters,
            0.0f,
            static_cast<float>(samplesZ - 1)
        );

        const auto x0 = static_cast<std::uint32_t>(gridX);
        const auto z0 = static_cast<std::uint32_t>(gridZ);

        const auto x1 = std::min(x0 + 1, samplesX - 1);
        const auto z1 = std::min(z0 + 1, samplesZ - 1);

        const float tx = gridX - static_cast<float>(x0);
        const float tz = gridZ - static_cast<float>(z0);

        const float top = std::lerp(
            height_at(x0, z0),
            height_at(x1, z0),
            tx
        );

        const float bottom = std::lerp(
            height_at(x0, z1),
            height_at(x1, z1),
            tx
        );

        return std::lerp(top, bottom, tz);
    }

    float Heightfield::slope_degrees(
        const float worldX,
        const float worldZ
    ) const {
        if (!valid() ||
            !std::isfinite(worldX) ||
            !std::isfinite(worldZ)) {
            throw std::invalid_argument(
                "Invalid terrain or world coordinates"
            );
        }

        const float maxX =
            originX + static_cast<float>(samplesX - 1) *
            spacingMeters;

        const float maxZ =
            originZ + static_cast<float>(samplesZ - 1) *
            spacingMeters;

        const float x = std::clamp(worldX, originX, maxX);
        const float z = std::clamp(worldZ, originZ, maxZ);

        const float x0 = std::max(
            originX, x - spacingMeters
        );

        const float x1 = std::min(
            maxX, x + spacingMeters
        );

        const float z0 = std::max(
            originZ, z - spacingMeters
        );

        const float z1 = std::min(
            maxZ, z + spacingMeters
        );

        const float dx = (
            sample_height(x1, z) -
            sample_height(x0, z)
        ) / (x1 - x0);

        const float dz = (
            sample_height(x, z1) -
            sample_height(x, z0)
        ) / (z1 - z0);

        return std::atan(std::hypot(dx, dz)) *
            (180.0f / std::numbers::pi_v<float>);
    }

}
