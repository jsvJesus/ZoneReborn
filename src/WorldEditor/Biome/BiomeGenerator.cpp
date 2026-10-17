#include "WorldEditor/Biome/BiomeGenerator.h"

#include "WorldEditor/Terrain/Noise.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace world_editor::biome {

    namespace {

        [[nodiscard]] float saturate(float value) noexcept {
            return std::clamp(value, 0.0f, 1.0f);
        }

        [[nodiscard]] float smoothstep(
            float minimum,
            float maximum,
            float value
        ) noexcept {
            const float t = saturate(
                (value - minimum) / (maximum - minimum)
            );

            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] std::size_t cell_index(
            std::uint32_t x,
            std::uint32_t z,
            std::uint32_t width
        ) noexcept {
            return static_cast<std::size_t>(z) * width + x;
        }

        [[nodiscard]] float sample_noise(
            float x,
            float z,
            float scale,
            std::uint32_t seed
        ) noexcept {
            return static_cast<float>(
                terrain::noise::fractal(
                    static_cast<double>(x / scale),
                    static_cast<double>(z / scale),
                    seed,
                    4
                )
            );
        }

        [[nodiscard]] bool near_water(
            const HydrologyLayer* water,
            std::uint32_t x,
            std::uint32_t z
        ) noexcept {
            if (!water) {
                return false;
            }

            for (int dz = -2; dz <= 2; ++dz) {
                for (int dx = -2; dx <= 2; ++dx) {
                    const int sx = static_cast<int>(x) + dx;
                    const int sz = static_cast<int>(z) + dz;

                    if (sx < 0 || sz < 0 ||
                        sx >= static_cast<int>(water->samplesX) ||
                        sz >= static_cast<int>(water->samplesZ)) {
                        continue;
                    }

                    const WaterKind kind = water->cells[
                        cell_index(
                            static_cast<std::uint32_t>(sx),
                            static_cast<std::uint32_t>(sz),
                            water->samplesX
                        )
                    ];

                    if (kind == WaterKind::River ||
                        kind == WaterKind::Lake) {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] float sample_slope(
            const terrain::Heightfield& heightfield,
            std::uint32_t x,
            std::uint32_t z
        ) noexcept {
            const std::uint32_t x0 = x > 0 ? x - 1 : x;
            const std::uint32_t z0 = z > 0 ? z - 1 : z;

            const std::uint32_t x1 = std::min(
                x + 1,
                heightfield.samplesX - 1
            );

            const std::uint32_t z1 = std::min(
                z + 1,
                heightfield.samplesZ - 1
            );

            const auto height = [&](std::uint32_t sx,
                                    std::uint32_t sz) {
                return heightfield.heights[
                    cell_index(sx, sz, heightfield.samplesX)
                ];
            };

            const float dx = (
                height(x1, z) - height(x0, z)
            ) / (
                static_cast<float>(x1 - x0) *
                heightfield.spacingMeters
            );

            const float dz = (
                height(x, z1) - height(x, z0)
            ) / (
                static_cast<float>(z1 - z0) *
                heightfield.spacingMeters
            );

            return std::atan(std::hypot(dx, dz)) *
                57.295779513f;
        }

        void store_weights(
            BiomeLayer& layer,
            std::size_t index,
            const std::array<float, 5>& source
        ) {
            float total = 0.0f;

            for (float weight : source) {
                total += weight;
            }

            const std::size_t offset = index * 4;

            if (total <= 0.00001f) {
                layer.materialWeights[offset] = 255;
                layer.materialWeights[offset + 1] = 0;
                layer.materialWeights[offset + 2] = 0;
                layer.materialWeights[offset + 3] = 0;
                return;
            }

            // The fifth material occupies the unused
            // normalized weight after four channels.
            for (std::size_t channel = 0; channel < 4; ++channel) {
                const float normalized =
                    saturate(source[channel] / total);

                layer.materialWeights[offset + channel] =
                    static_cast<std::uint8_t>(
                        std::floor(normalized * 255.0f)
                    );
            }
        }

    }

    bool BiomeLayer::valid() const noexcept {
        const std::size_t count =
            static_cast<std::size_t>(samplesX) * samplesZ;

        return samplesX >= 2 &&
            samplesZ >= 2 &&
            std::isfinite(originX) &&
            std::isfinite(originZ) &&
            std::isfinite(spacingMeters) &&
            spacingMeters > 0.0f &&
            zones.size() == count &&
            materialWeights.size() == count * 4;
    }

    BiomeKind BiomeLayer::at_world(
        float worldX,
        float worldZ
    ) const noexcept {
        if (!valid() ||
            !std::isfinite(worldX) ||
            !std::isfinite(worldZ)) {
            return BiomeKind::Grassland;
        }

        const float gx = std::clamp(
            (worldX - originX) / spacingMeters,
            0.0f,
            static_cast<float>(samplesX - 1)
        );

        const float gz = std::clamp(
            (worldZ - originZ) / spacingMeters,
            0.0f,
            static_cast<float>(samplesZ - 1)
        );

        return zones[cell_index(
            static_cast<std::uint32_t>(std::round(gx)),
            static_cast<std::uint32_t>(std::round(gz)),
            samplesX
        )];
    }

    BiomeLayer generate_biomes(
        const terrain::Heightfield& heightfield,
        const HydrologyLayer* hydrology,
        const BiomeSettings& settings
    ) {
        if (!heightfield.valid() ||
            !std::isfinite(settings.moisture) ||
            !std::isfinite(settings.forestCoverage) ||
            !std::isfinite(settings.rockSlopeDegrees) ||
            !std::isfinite(settings.textureScaleMeters) ||
            settings.moisture < 0.0f ||
            settings.moisture > 1.0f ||
            settings.forestCoverage < 0.0f ||
            settings.forestCoverage > 1.0f ||
            settings.rockSlopeDegrees < 5.0f ||
            settings.rockSlopeDegrees > 60.0f ||
            settings.textureScaleMeters <= 0.0f) {
            throw std::invalid_argument(
                "Invalid Biome Generator settings"
            );
        }

        if (hydrology) {
            if (!hydrology->valid() ||
                hydrology->samplesX != heightfield.samplesX ||
                hydrology->samplesZ != heightfield.samplesZ ||
                hydrology->originX != heightfield.originX ||
                hydrology->originZ != heightfield.originZ ||
                hydrology->spacingMeters !=
                    heightfield.spacingMeters) {
                throw std::invalid_argument(
                    "Hydrology does not match Terrain"
                );
            }
        }

        BiomeLayer layer;

        layer.samplesX = heightfield.samplesX;
        layer.samplesZ = heightfield.samplesZ;
        layer.originX = heightfield.originX;
        layer.originZ = heightfield.originZ;
        layer.spacingMeters = heightfield.spacingMeters;
        layer.settings = settings;

        const std::size_t count = heightfield.heights.size();

        layer.zones.resize(count);
        layer.materialWeights.resize(count * 4);

        const auto [minimum, maximum] = std::minmax_element(
            heightfield.heights.begin(),
            heightfield.heights.end()
        );

        const float heightRange = std::max(
            *maximum - *minimum,
            1.0f
        );

        for (std::uint32_t z = 0; z < layer.samplesZ; ++z) {
            for (std::uint32_t x = 0; x < layer.samplesX; ++x) {
                const std::size_t index =
                    cell_index(x, z, layer.samplesX);

                const float worldX = layer.originX +
                    static_cast<float>(x) * layer.spacingMeters;

                const float worldZ = layer.originZ +
                    static_cast<float>(z) * layer.spacingMeters;

                const float height = heightfield.heights[index];

                const float elevation = saturate(
                    (height - *minimum) / heightRange
                );

                const float slope = sample_slope(
                    heightfield, x, z
                );

                const float macro = sample_noise(
                    worldX,
                    worldZ,
                    280.0f,
                    settings.seed
                );

                const float detail = sample_noise(
                    worldX,
                    worldZ,
                    68.0f,
                    settings.seed ^ 0x9E3779B9u
                );

                const float vegetationNoise = sample_noise(
                    worldX,
                    worldZ,
                    190.0f,
                    settings.seed ^ 0x85EBCA6Bu
                );

                const WaterKind waterKind = hydrology
                    ? hydrology->cells[index]
                    : WaterKind::Dry;

                const bool shoreline = near_water(
                    hydrology, x, z
                );

                const float moisture = saturate(
                    settings.moisture +
                    macro * 0.30f +
                    detail * 0.18f -
                    elevation * 0.28f +
                    (shoreline ? 0.32f : 0.0f)
                );

                const float rock = smoothstep(
                    settings.rockSlopeDegrees * 0.55f,
                    settings.rockSlopeDegrees * 1.35f,
                    slope
                );

                const float dryness = 1.0f - moisture;

                float grassWeight =
                    (0.75f + moisture * 0.30f) *
                    (1.0f - rock);

                float soilWeight =
                    0.22f + dryness * 0.42f +
                    rock * 0.10f;

                float sandWeight =
                    dryness * dryness * 0.60f +
                    (shoreline ? 0.75f : 0.0f);

                float rockWeight = rock * 2.20f;

                float marshWeight = smoothstep(
                    0.63f,
                    0.94f,
                    moisture
                ) * 0.80f;

                if (waterKind == WaterKind::Marsh) {
                    grassWeight = 0.12f;
                    soilWeight = 0.32f;
                    sandWeight = 0.08f;
                    rockWeight = 0.0f;
                    marshWeight = 3.5f;
                }

                if (waterKind == WaterKind::River ||
                    waterKind == WaterKind::Lake) {
                    grassWeight = 0.0f;
                    soilWeight = 0.45f;
                    sandWeight = 0.65f;
                    rockWeight = 0.0f;
                    marshWeight = 1.1f;
                }

                store_weights(
                    layer,
                    index,
                    {
                        grassWeight,
                        soilWeight,
                        sandWeight,
                        rockWeight,
                        marshWeight
                    }
                );

                BiomeKind biome = BiomeKind::Grassland;

                if (waterKind != WaterKind::Dry ||
                    (moisture > 0.83f && slope < 12.0f)) {
                    biome = BiomeKind::Wetland;
                } else if (rock > 0.60f) {
                    biome = BiomeKind::Rocky;
                } else if (sandWeight > grassWeight &&
                           moisture < 0.50f) {
                    biome = BiomeKind::Sandy;
                } else if (
                    moisture > 0.39f &&
                    slope < 17.0f &&
                    vegetationNoise >
                        (0.38f - settings.forestCoverage * 0.76f)
                ) {
                    biome = BiomeKind::Forest;
                }

                layer.zones[index] = biome;
            }
        }

        return layer;
    }

}
