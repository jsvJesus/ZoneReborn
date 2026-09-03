#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::world
{
    struct TerrainHeightData final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::uint32_t visibleOffset = 2;

        float minHeight = 0.0f;
        float maxHeight = 0.0f;

        std::vector<float> heights;

        [[nodiscard]]
        float At(
            const std::uint32_t x,
            const std::uint32_t z) const noexcept
        {
            if (x >= width ||
                z >= height)
            {
                return 0.0f;
            }

            return heights[
                static_cast<std::size_t>(z) *
                    width +
                x];
        }

        [[nodiscard]]
        std::uint32_t VisibleWidth() const noexcept
        {
            if (width <=
                visibleOffset * 2)
            {
                return 0;
            }

            return
                width -
                visibleOffset * 2;
        }

        [[nodiscard]]
        std::uint32_t VisibleHeight() const noexcept
        {
            if (height <=
                visibleOffset * 2)
            {
                return 0;
            }

            return
                height -
                visibleOffset * 2;
        }
    };
}