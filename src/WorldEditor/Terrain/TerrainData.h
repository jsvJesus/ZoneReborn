#pragma once

#include <cstdint>
#include <vector>

namespace world_editor::terrain {

    struct Settings {
        std::uint32_t samplesX = 257;
        std::uint32_t samplesZ = 257;
        std::uint32_t seed = 20261016;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 8.0f;
        float baseHeightMeters = 120.0f;
        float reliefMeters = 18.0f;
        float macroScaleMeters = 900.0f;
        float detailScaleMeters = 120.0f;
    };

    struct Heightfield {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        std::vector<float> heights;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] float height_at(
            std::uint32_t x,
            std::uint32_t z
        ) const;

        [[nodiscard]] float sample_height(
            float worldX,
            float worldZ
        ) const;

        [[nodiscard]] float slope_degrees(
            float worldX,
            float worldZ
        ) const;
    };

}
