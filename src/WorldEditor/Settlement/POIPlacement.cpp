#include "WorldEditor/Settlement/POIPlacement.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace world_editor::settlement {

    POISpecification specification(const POIType type) {
        switch (type) {
        case POIType::Checkpoint:
            return {64.0f, 64.0f, 16.0f, 512};
        case POIType::Farm:
            return {112.0f, 112.0f, 12.0f, 512};
        case POIType::Village:
            return {224.0f, 192.0f, 9.0f, 1024};
        case POIType::Industrial:
            return {320.0f, 256.0f, 8.0f, 2048};
        case POIType::Town:
            return {768.0f, 768.0f, 7.0f, 2048};
        }

        throw std::invalid_argument("Unknown POI type");
    }

    float maximum_earthwork_height(
        const POISpecification& spec
    ) noexcept {
        return std::max(
            24.0f,
            std::min(spec.widthMeters, spec.depthMeters) *
                std::tan(spec.maxSlopeDegrees * 2.0f *
                    std::numbers::pi_v<float> / 180.0f)
        );
    }

}
