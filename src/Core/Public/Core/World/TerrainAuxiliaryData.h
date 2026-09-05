#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::world
{
    struct TerrainHoleData final
    {
        bool present =
            false;

        std::uint32_t width =
            0;

        std::uint32_t height =
            0;

        std::vector<std::uint8_t>
            cells;

        [[nodiscard]]
        bool IsHole(
            const std::uint32_t x,
            const std::uint32_t z) const noexcept
        {
            if (!present ||
                x >= width ||
                z >= height)
            {
                return false;
            }

            return
                cells[
                    static_cast<std::size_t>(
                        z) *
                        width +
                    x] !=
                0;
        }

        [[nodiscard]]
        std::size_t HoleCount() const noexcept
        {
            std::size_t count =
                0;

            for (const std::uint8_t value :
                 cells)
            {
                if (value != 0)
                {
                    ++count;
                }
            }

            return count;
        }
    };

    struct TerrainHorizonShadowData final
    {
        bool present =
            false;

        std::uint32_t width =
            0;

        std::uint32_t height =
            0;

        std::vector<std::uint32_t>
            values;
    };

    struct TerrainDominantTextureData final
    {
        bool present =
            false;

        std::uint32_t width =
            0;

        std::uint32_t height =
            0;

        std::vector<std::string>
            textureReferences;

        std::vector<std::uint8_t>
            indices;
    };

    struct TerrainAuxiliaryData final
    {
        TerrainHoleData
            holes;

        TerrainHorizonShadowData
            horizonShadows;

        TerrainDominantTextureData
            dominantTextures;

        std::vector<std::byte>
            lodTextureDds;
    };
}