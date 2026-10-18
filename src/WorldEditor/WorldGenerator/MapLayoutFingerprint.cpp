#include "WorldEditor/WorldGenerator/MapLayoutFingerprint.h"

#include "WorldEditor/Biome/BiomeStorage.h"

#include <bit>

namespace world_editor::world_generator {

    std::uint64_t layout_source_fingerprint(
        std::uint64_t terrainFingerprint,
        const biome::HydrologyLayer* water
    ) {
        std::uint64_t hash = biome::biome_source_fingerprint(terrainFingerprint, water);

        if (!water) {
            return hash;
        }

        const auto mix = [&hash](std::uint32_t value) {
            for (std::uint32_t i = 0; i < 4; ++i) {
                hash ^= (value >> (i * 8)) & 0xFFu;
                hash *= 1099511628211ull;
            }
        };

        const auto point = [&mix](const biome::WaterPoint& value) {
            mix(std::bit_cast<std::uint32_t>(value.x));
            mix(std::bit_cast<std::uint32_t>(value.y));
            mix(std::bit_cast<std::uint32_t>(value.z));
        };

        const auto channel = [&mix, &point](const auto& points) {
            mix(static_cast<std::uint32_t>(points.size()));

            for (const auto& value : points) {
                point(value);
            }
        };

        const auto& settings = water->settings;
        mix(settings.seed);
        mix(settings.riverEnabled ? 1u : 0u);
        mix(static_cast<std::uint32_t>(settings.riverShape));
        mix(settings.lakeCount);
        mix(std::bit_cast<std::uint32_t>(settings.riverWidthMeters));
        mix(std::bit_cast<std::uint32_t>(settings.marshWidthMeters));
        mix(std::bit_cast<std::uint32_t>(settings.lakeSeparationMeters));
        channel(water->river);
        mix(static_cast<std::uint32_t>(water->tributaries.size()));

        for (const auto& branch : water->tributaries) {
            channel(branch);
        }

        mix(static_cast<std::uint32_t>(water->lakes.size()));

        for (const auto& lake : water->lakes) {
            point(lake.center);
            mix(std::bit_cast<std::uint32_t>(lake.radiusMeters));
        }

        channel(water->marshMarkers);
        return hash;
    }

}
