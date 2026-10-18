#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include "WorldEditor/Terrain/Noise.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>

namespace world_editor::vegetation {

    namespace {

        [[nodiscard]] float saturate(float value) noexcept {
            return std::clamp(value, 0.0f, 1.0f);
        }

        [[nodiscard]] std::size_t index_of(
            std::uint32_t x,
            std::uint32_t z,
            std::uint32_t width
        ) noexcept {
            return static_cast<std::size_t>(z) * width + x;
        }

        [[nodiscard]] std::uint32_t hash_value(
            std::uint32_t seed,
            std::uint32_t x,
            std::uint32_t z,
            std::uint32_t salt
        ) noexcept {
            std::uint32_t h = seed;

            h ^= x * 0x9E3779B9u;
            h ^= z * 0x85EBCA6Bu;
            h ^= salt * 0xC2B2AE35u;

            h ^= h >> 16;
            h *= 0x7FEB352Du;
            h ^= h >> 15;
            h *= 0x846CA68Bu;
            h ^= h >> 16;

            return h;
        }

        [[nodiscard]] float random01(
            std::uint32_t seed,
            std::uint32_t x,
            std::uint32_t z,
            std::uint32_t salt
        ) noexcept {
            const std::uint32_t value =
                hash_value(seed, x, z, salt) >> 8;

            return static_cast<float>(value) *
                (1.0f / 16777216.0f);
        }

        [[nodiscard]] bool same_grid(
            const terrain::Heightfield& terrain,
            const biome::BiomeLayer& biomes
        ) noexcept {
            return terrain.samplesX == biomes.samplesX &&
                terrain.samplesZ == biomes.samplesZ &&
                terrain.originX == biomes.originX &&
                terrain.originZ == biomes.originZ &&
                terrain.spacingMeters == biomes.spacingMeters;
        }

        [[nodiscard]] bool same_grid(
            const terrain::Heightfield& terrain,
            const biome::HydrologyLayer& water
        ) noexcept {
            return terrain.samplesX == water.samplesX &&
                terrain.samplesZ == water.samplesZ &&
                terrain.originX == water.originX &&
                terrain.originZ == water.originZ &&
                terrain.spacingMeters == water.spacingMeters;
        }

        [[nodiscard]] float distance_to_segment_squared(
            float x,
            float z,
            float ax,
            float az,
            float bx,
            float bz
        ) noexcept {
            const float dx = bx - ax;
            const float dz = bz - az;

            const float lengthSquared = dx * dx + dz * dz;

            if (lengthSquared < 0.0001f) {
                const float px = x - ax;
                const float pz = z - az;

                return px * px + pz * pz;
            }

            const float t = saturate(
                ((x - ax) * dx + (z - az) * dz) /
                lengthSquared
            );

            const float px = x - (ax + dx * t);
            const float pz = z - (az + dz * t);

            return px * px + pz * pz;
        }

        [[nodiscard]] bool blocked_by_layout(
            float x,
            float z,
            const world_generator::MapLayout* layout,
            const VegetationSettings& settings
        ) noexcept {
            if (!layout) {
                return false;
            }

            for (const auto& poi : layout->pois) {
                const float halfWidth =
                    poi.widthMeters * 0.5f +
                    settings.poiClearanceMeters;

                const float halfDepth =
                    poi.depthMeters * 0.5f +
                    settings.poiClearanceMeters;

                if (std::abs(x - poi.centerX) <= halfWidth &&
                    std::abs(z - poi.centerZ) <= halfDepth) {
                    return true;
                }
            }

            const float exitRadiusSquared =
                settings.exitClearanceMeters *
                settings.exitClearanceMeters;

            for (const auto& exit : layout->exits) {
                const float dx = x - exit.x;
                const float dz = z - exit.z;

                if (dx * dx + dz * dz <= exitRadiusSquared) {
                    return true;
                }
            }

            const auto& graph = layout->roads;

            const float roadRadiusSquared =
                settings.roadClearanceMeters *
                settings.roadClearanceMeters;

            for (const auto& edge : graph.edges) {
                if (edge.from >= graph.nodes.size() ||
                    edge.to >= graph.nodes.size()) {
                    continue;
                }

                const auto& a = graph.nodes[edge.from];
                const auto& b = graph.nodes[edge.to];

                if (distance_to_segment_squared(
                    x, z,
                    a.x, a.z,
                    b.x, b.z
                ) <= roadRadiusSquared) {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool blocked_by_water(
            float x,
            float z,
            const biome::HydrologyLayer* hydrology
        ) noexcept {
            if (!hydrology) {
                return false;
            }

            const auto kind = hydrology->at_world(x, z);

            return kind == biome::WaterKind::River ||
                kind == biome::WaterKind::Lake;
        }

        [[nodiscard]] std::array<float, 3> biome_densities(
            biome::BiomeKind kind
        ) noexcept {
            switch (kind) {
            case biome::BiomeKind::Grassland:
                return {0.78f, 0.12f, 0.045f};

            case biome::BiomeKind::Forest:
                return {0.64f, 0.28f, 0.43f};

            case biome::BiomeKind::Sandy:
                return {0.10f, 0.035f, 0.005f};

            case biome::BiomeKind::Rocky:
                return {0.035f, 0.010f, 0.005f};

            case biome::BiomeKind::Wetland:
                return {0.55f, 0.35f, 0.085f};
            }

            return {};
        }

        [[nodiscard]] VegetationAsset choose_asset(
            VegetationKind kind,
            biome::BiomeKind biomeKind,
            float variation
        ) noexcept {
            if (kind == VegetationKind::Grass) {
                if (biomeKind == biome::BiomeKind::Wetland) {
                    return VegetationAsset::SwampGrass;
                }

                if (biomeKind == biome::BiomeKind::Sandy ||
                    variation > 0.80f) {
                    return VegetationAsset::DryGrass;
                }

                return VegetationAsset::FieldGrass;
            }

            if (kind == VegetationKind::Bush) {
                if (biomeKind == biome::BiomeKind::Wetland) {
                    return VegetationAsset::MarshReeds;
                }

                return VegetationAsset::GreenBush;
            }

            if (biomeKind == biome::BiomeKind::Wetland) {
                return VegetationAsset::SwampTree;
            }

            return variation < 0.55f
                ? VegetationAsset::FirTree
                : VegetationAsset::PineTree;
        }

        [[nodiscard]] bool allows_slope(
            VegetationKind kind,
            float slope
        ) noexcept {
            switch (kind) {
            case VegetationKind::Grass:
                return slope <= 34.0f;

            case VegetationKind::Bush:
                return slope <= 27.0f;

            case VegetationKind::Tree:
                return slope <= 19.0f;
            }

            return false;
        }

        [[nodiscard]] float density_noise(
            float x,
            float z,
            std::uint32_t seed
        ) noexcept {
            const float value = static_cast<float>(
                terrain::noise::fractal(
                    static_cast<double>(x) / 170.0,
                    static_cast<double>(z) / 170.0,
                    seed,
                    3
                )
            );

            return saturate(0.76f + value * 0.50f);
        }

        [[nodiscard]] std::uint8_t to_mask(
            float value
        ) noexcept {
            return static_cast<std::uint8_t>(
                std::lround(saturate(value) * 255.0f)
            );
        }

    }

    std::string_view asset_relative_path(
        VegetationAsset asset
    ) noexcept {
        switch (asset) {
        case VegetationAsset::FieldGrass:
            return "models/flora/grass/field_grass/"
                "field_grass_1.primitives";

        case VegetationAsset::DryGrass:
            return "models/flora/grass/"
                "field_grass_long_yellow.primitives";

        case VegetationAsset::SwampGrass:
            return "models/flora/grass/swamp_grass/"
                "swamp_grass_1.primitives";

        case VegetationAsset::GreenBush:
            return "models/flora/grass/"
                "bush_01a_green.primitives";

        case VegetationAsset::MarshReeds:
            return "models/flora/grass/"
                "swamp_kamysh_01.primitives";

        case VegetationAsset::FirTree:
            return "models/flora/trees/"
                "furtree_01c.primitives";

        case VegetationAsset::PineTree:
            return "models/flora/trees/"
                "pi_tree_01a.primitives";

        case VegetationAsset::SwampTree:
            return "models/flora/swamp_trees/"
                "swamp_tree_01.primitives";

        case VegetationAsset::Count:
            break;
        }

        return {};
    }

    bool VegetationLayer::valid() const noexcept {
        const std::size_t count =
            static_cast<std::size_t>(samplesX) * samplesZ;

        return samplesX >= 2 &&
            samplesZ >= 2 &&
            std::isfinite(originX) &&
            std::isfinite(originZ) &&
            std::isfinite(spacingMeters) &&
            spacingMeters > 0.0f &&
            grassMask.size() == count &&
            bushMask.size() == count &&
            treeMask.size() == count &&
            instances.size() <= settings.maxInstances;
    }

    VegetationLayer generate_vegetation(
        const terrain::Heightfield& terrain,
        const biome::BiomeLayer& biomes,
        const biome::HydrologyLayer* hydrology,
        const world_generator::MapLayout* layout,
        const VegetationSettings& settings
    ) {
        if (!terrain.valid() ||
            !biomes.valid() ||
            !same_grid(terrain, biomes)) {
            throw std::invalid_argument(
                "Vegetation requires matching Terrain and Biomes"
            );
        }

        if (hydrology &&
            (!hydrology->valid() ||
             !same_grid(terrain, *hydrology))) {
            throw std::invalid_argument(
                "Hydrology grid does not match Terrain"
            );
        }

        const auto validDensity = [](float value) {
            return std::isfinite(value) &&
                value >= 0.0f &&
                value <= 1.0f;
        };

        if (!validDensity(settings.grassDensity) ||
            !validDensity(settings.bushDensity) ||
            !validDensity(settings.treeDensity) ||
            !std::isfinite(settings.roadClearanceMeters) ||
            !std::isfinite(settings.poiClearanceMeters) ||
            !std::isfinite(settings.exitClearanceMeters) ||
            settings.roadClearanceMeters < 0.0f ||
            settings.poiClearanceMeters < 0.0f ||
            settings.exitClearanceMeters < 0.0f ||
            settings.maxInstances == 0 ||
            settings.maxInstances > 500000) {
            throw std::invalid_argument(
                "Invalid Vegetation Generator settings"
            );
        }

        VegetationLayer layer;

        layer.samplesX = terrain.samplesX;
        layer.samplesZ = terrain.samplesZ;
        layer.originX = terrain.originX;
        layer.originZ = terrain.originZ;
        layer.spacingMeters = terrain.spacingMeters;
        layer.settings = settings;

        const std::size_t sampleCount =
            terrain.heights.size();

        layer.grassMask.resize(sampleCount);
        layer.bushMask.resize(sampleCount);
        layer.treeMask.resize(sampleCount);

        // Independent deterministic thinning prevents large maps
        // from concentrating instances near the first rows.
        const float budgetFactor = (std::min)(
            1.0f,
            static_cast<float>(settings.maxInstances) /
            (
                static_cast<float>(sampleCount) *
                1.8f
            )
        );

        constexpr std::array<VegetationKind, 3> kinds = {
            VegetationKind::Grass,
            VegetationKind::Bush,
            VegetationKind::Tree
        };

        constexpr float twoPi =
            2.0f * std::numbers::pi_v<float>;

        for (std::uint32_t z = 0; z < layer.samplesZ; ++z) {
            for (std::uint32_t x = 0; x < layer.samplesX; ++x) {
                const std::size_t index = index_of(
                    x, z, layer.samplesX
                );

                const float worldX = layer.originX +
                    static_cast<float>(x) *
                    layer.spacingMeters;

                const float worldZ = layer.originZ +
                    static_cast<float>(z) *
                    layer.spacingMeters;

                const auto biomeKind = biomes.zones[index];

                const float slope = terrain.slope_degrees(
                    worldX, worldZ
                );

                const float patch = density_noise(
                    worldX,
                    worldZ,
                    settings.seed
                );

                auto densities = biome_densities(biomeKind);

                densities[0] *= settings.grassDensity;
                densities[1] *= settings.bushDensity;
                densities[2] *= settings.treeDensity;

                for (float& density : densities) {
                    density *= patch;
                }

                if (blocked_by_water(
                    worldX, worldZ, hydrology
                ) || blocked_by_layout(
                    worldX, worldZ, layout, settings
                )) {
                    densities = {};
                }

                for (std::size_t i = 0; i < kinds.size(); ++i) {
                    if (!allows_slope(kinds[i], slope)) {
                        densities[i] = 0.0f;
                    }
                }

                layer.grassMask[index] =
                    to_mask(densities[0]);

                layer.bushMask[index] =
                    to_mask(densities[1]);

                layer.treeMask[index] =
                    to_mask(densities[2]);

                // The outermost row/column are grid boundaries,
                // not independent placement cells.
                if (x + 1 >= layer.samplesX ||
                    z + 1 >= layer.samplesZ) {
                    continue;
                }

                for (std::size_t i = 0; i < kinds.size(); ++i) {
                    const auto kind = kinds[i];

                    const std::uint32_t salt =
                        static_cast<std::uint32_t>(i) * 17u;

                    const float probability =
                        densities[i] * budgetFactor;

                    if (random01(
                        settings.seed, x, z, salt + 1u
                    ) >= probability) {
                        continue;
                    }

                    const float offsetX =
                        0.15f + random01(
                            settings.seed, x, z, salt + 2u
                        ) * 0.70f;

                    const float offsetZ =
                        0.15f + random01(
                            settings.seed, x, z, salt + 3u
                        ) * 0.70f;

                    const float px =
                        worldX +
                        offsetX * layer.spacingMeters;

                    const float pz =
                        worldZ +
                        offsetZ * layer.spacingMeters;

                    if (blocked_by_water(
                        px, pz, hydrology
                    ) || blocked_by_layout(
                        px, pz, layout, settings
                    )) {
                        continue;
                    }

                    if (!allows_slope(
                        kind,
                        terrain.slope_degrees(px, pz)
                    )) {
                        continue;
                    }

                    const float variation = random01(
                        settings.seed, x, z, salt + 4u
                    );

                    VegetationInstance instance;

                    instance.x = px;
                    instance.y = terrain.sample_height(px, pz);
                    instance.z = pz;

                    instance.yawRadians = twoPi * random01(
                        settings.seed, x, z, salt + 5u
                    );

                    instance.scale =
                        0.78f + random01(
                            settings.seed, x, z, salt + 6u
                        ) * 0.44f;

                    instance.kind = kind;
                    instance.asset = choose_asset(
                        kind, biomeKind, variation
                    );

                    if (layer.instances.size() <
                        settings.maxInstances) {
                        layer.instances.push_back(instance);
                    }
                }
            }
        }

        return layer;
    }

}
