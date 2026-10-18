#pragma once

#include <cstdint>

namespace editor::world_generator {

    struct LayerGrid {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;
        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;

        [[nodiscard]] bool operator==(const LayerGrid&) const = default;
    };

    template<typename Layer>
    [[nodiscard]] LayerGrid layer_grid(const Layer& layer) noexcept {
        return {
            layer.samplesX, layer.samplesZ,
            layer.originX, layer.originZ, layer.spacingMeters
        };
    }

    struct LayerRevisions {
        std::uint64_t terrain = 0;
        std::uint64_t hydrology = 0;
        std::uint64_t layout = 0;
        std::uint64_t biomes = 0;

        [[nodiscard]] bool operator==(const LayerRevisions&) const = default;
    };

}
