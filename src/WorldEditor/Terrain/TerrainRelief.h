#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <cstdint>

namespace world_editor::terrain {

    enum class ReliefPreset : std::uint32_t {
        Original,
        RollingHills,
        Highlands,
        Mountains,
        Mixed
    };

    struct ReliefProfile {
        ReliefPreset preset = ReliefPreset::Original;

        float hillHeightMeters = 0.0f;
        float mountainHeightMeters = 0.0f;
        float mountainCoverage = 0.0f;
        float ridgeSharpness = 1.5f;
    };

    [[nodiscard]] ReliefProfile relief_preset(
        ReliefPreset preset
    ) noexcept;

    [[nodiscard]] Heightfield generate_with_relief(
        const Settings& settings,
        const ReliefProfile& profile
    );

}
