#pragma once

#include "Core/Assets/TextureResource.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::world
{
    struct TerrainLayerData final
    {
        std::string sectionName;

        assets::TextureResource texture;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::array<float, 4>
            uProjection{};

        std::array<float, 4>
            vProjection{};

        std::vector<std::uint8_t>
            blend;

        [[nodiscard]]
        std::uint8_t BlendAt(
            const std::uint32_t x,
            const std::uint32_t z) const noexcept
        {
            if (x >= width ||
                z >= height)
            {
                return 0;
            }

            return blend[
                static_cast<std::size_t>(z) *
                    width +
                x];
        }
    };
}