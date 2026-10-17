#pragma once

#include "WorldEditor/Settlement/POIPlacement.h"

#include <cstdint>
#include <vector>

namespace world_editor::world_generator {

    enum class MapSize : std::uint32_t {
        Small = 512,
        Medium = 1024,
        Large = 2048,
        ExtraLarge = 4096
    };

    struct GenerationSeeds {
        std::uint32_t terrain = 20261016;
        std::uint32_t water = 20261017;
        std::uint32_t vegetation = 20261018;
        std::uint32_t location = 20261019;
        std::uint32_t road = 20261020;
        std::uint32_t decoration = 20261021;
    };

    struct MapGenerationSettings {
        MapSize mapSize = MapSize::ExtraLarge;
        std::uint32_t poiCount = 5;
        std::uint32_t exitCount = 2;
        float poiSeparationMeters = 48.0f;
        float borderClearanceMeters = 32.0f;
        GenerationSeeds seeds;
        std::vector<settlement::POIType> allowedTypes;
    };

    [[nodiscard]] constexpr std::uint32_t map_size_meters(
        const MapSize size
    ) noexcept {
        return static_cast<std::uint32_t>(size);
    }

}
