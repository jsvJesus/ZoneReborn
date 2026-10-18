#pragma once

#include <cstdint>

namespace world_editor::biome {
    struct HydrologyLayer;
}

namespace world_editor::world_generator {

    [[nodiscard]] std::uint64_t layout_source_fingerprint(
        std::uint64_t terrainFingerprint,
        const biome::HydrologyLayer* water
    );

}
