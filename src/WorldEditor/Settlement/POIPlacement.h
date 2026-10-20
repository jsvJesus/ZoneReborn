#pragma once

#include <cstdint>

namespace world_editor::settlement {

    enum class POIType : std::uint32_t {
        Checkpoint,
        Farm,
        Village,
        Industrial,
        Town
    };

    struct POISpecification {
        float widthMeters = 0.0f;
        float depthMeters = 0.0f;
        float maxSlopeDegrees = 0.0f;
        std::uint32_t minimumMapMeters = 0;
    };

    struct POIPlacement {
        std::uint32_t id = 0;
        POIType type = POIType::Checkpoint;
        float centerX = 0.0f;
        float centerY = 0.0f;
        float centerZ = 0.0f;
        float widthMeters = 0.0f;
        float depthMeters = 0.0f;
    };

    [[nodiscard]] POISpecification specification(POIType type);

    [[nodiscard]] float maximum_earthwork_height(
        const POISpecification& specification
    ) noexcept;

}
