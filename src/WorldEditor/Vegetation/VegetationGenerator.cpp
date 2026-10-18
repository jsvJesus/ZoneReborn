#include "WorldEditor/Vegetation/VegetationGenerator.h"
#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"

#include "WorldEditor/Terrain/Noise.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <limits>
#include <vector>

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

        struct RoadSegment {
            float ax = 0.0f;
            float az = 0.0f;
            float bx = 0.0f;
            float bz = 0.0f;
        };

        class RoadIndex final {
        public:
            RoadIndex(
                const terrain::Heightfield& terrain,
                const world_generator::MapLayout* layout,
                float clearance
            ) : originX_(terrain.originX),
                originZ_(terrain.originZ),
                clearanceSquared_(clearance * clearance) {
                if (!layout) {
                    return;
                }

                constexpr float bucketSize = 64.0f;

                columns_ = static_cast<std::uint32_t>(
                    std::ceil(
                        (terrain.samplesX - 1) * terrain.spacingMeters /
                        bucketSize
                    )
                ) + 1;

                rows_ = static_cast<std::uint32_t>(
                    std::ceil(
                        (terrain.samplesZ - 1) * terrain.spacingMeters /
                        bucketSize
                    )
                ) + 1;

                buckets_.resize(
                    static_cast<std::size_t>(columns_) * rows_
                );

                for (const auto& edge : layout->roads.edges) {
                    for (std::size_t i = 1; i < edge.route.size(); ++i) {
                        const auto& a = edge.route[i - 1];
                        const auto& b = edge.route[i];

                        const auto firstX = bucket_x(
                            (std::min)(a.x, b.x) - clearance
                        );
                        const auto lastX = bucket_x(
                            (std::max)(a.x, b.x) + clearance
                        );
                        const auto firstZ = bucket_z(
                            (std::min)(a.z, b.z) - clearance
                        );
                        const auto lastZ = bucket_z(
                            (std::max)(a.z, b.z) + clearance
                        );

                        for (std::uint32_t z = firstZ; z <= lastZ; ++z) {
                            for (std::uint32_t x = firstX; x <= lastX; ++x) {
                                buckets_[
                                    static_cast<std::size_t>(z) * columns_ + x
                                ].push_back({a.x, a.z, b.x, b.z});
                            }
                        }
                    }
                }
            }

            [[nodiscard]] bool blocks(float x, float z) const noexcept {
                if (buckets_.empty()) {
                    return false;
                }

                const auto& bucket = buckets_[
                    static_cast<std::size_t>(bucket_z(z)) * columns_ +
                    bucket_x(x)
                ];

                for (const auto& segment : bucket) {
                    if (distance_to_segment_squared(
                        x, z,
                        segment.ax, segment.az,
                        segment.bx, segment.bz
                    ) <= clearanceSquared_) {
                        return true;
                    }
                }

                return false;
            }

        private:
            [[nodiscard]] std::uint32_t bucket_x(float x) const noexcept {
                return static_cast<std::uint32_t>(std::clamp(
                    (x - originX_) / 64.0f,
                    0.0f,
                    static_cast<float>(columns_ - 1)
                ));
            }

            [[nodiscard]] std::uint32_t bucket_z(float z) const noexcept {
                return static_cast<std::uint32_t>(std::clamp(
                    (z - originZ_) / 64.0f,
                    0.0f,
                    static_cast<float>(rows_ - 1)
                ));
            }

            float originX_ = 0.0f;
            float originZ_ = 0.0f;
            float clearanceSquared_ = 0.0f;
            std::uint32_t columns_ = 0;
            std::uint32_t rows_ = 0;
            std::vector<std::vector<RoadSegment>> buckets_;
        };

        [[nodiscard]] bool blocked_by_layout(
            float x,
            float z,
            const world_generator::MapLayout* layout,
            const VegetationSettings& settings,
            const RoadIndex& roads
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

            return roads.blocks(x, z);
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

        struct WeightedAsset {
            std::uint32_t index = 0;
            float cumulativeWeight = 0.0f;
        };

        struct WeightedAssetPool {
            std::vector<WeightedAsset> assets;
            float totalWeight = 0.0f;

            [[nodiscard]] bool empty() const noexcept {
                return assets.empty();
            }

            void add(
                std::uint32_t index,
                float weight
            ) {
                totalWeight += weight;

                assets.push_back({
                    index,
                    totalWeight
                });
            }

            [[nodiscard]] std::uint32_t pick(
                float randomValue
            ) const {
                if (assets.empty()) {
                    throw std::logic_error(
                        "Vegetation asset pool is empty"
                    );
                }

                const float target =
                    randomValue * totalWeight;

                const auto found = std::lower_bound(
                    assets.begin(),
                    assets.end(),
                    target,
                    [](const WeightedAsset& asset,
                       float value) {
                        return asset.cumulativeWeight <= value;
                    }
                );

                return found == assets.end()
                    ? assets.back().index
                    : found->index;
            }
        };

        using AssetPools = std::array<std::array<WeightedAssetPool, 5>, 3>;

        [[nodiscard]] bool matches_habitat(
            VegetationHabitat habitat,
            biome::BiomeKind biomeKind
        ) noexcept {
            if (habitat == VegetationHabitat::Any) {
                return true;
            }

            switch (biomeKind) {
            case biome::BiomeKind::Grassland:
                return habitat == VegetationHabitat::Grassland;

            case biome::BiomeKind::Forest:
                return habitat == VegetationHabitat::Forest;

            case biome::BiomeKind::Sandy:
                return habitat == VegetationHabitat::Sandy;

            case biome::BiomeKind::Rocky:
                return habitat == VegetationHabitat::Rocky;

            case biome::BiomeKind::Wetland:
                return habitat == VegetationHabitat::Wetland;
            }

            return false;
        }

        [[nodiscard]] AssetPools build_asset_pools(
            const VegetationAssetCatalog& catalog
        )
        {
            AssetPools pools{};

            for (std::size_t kind = 0; kind < 3; ++kind) {
                for (std::size_t biomeIndex = 0;
                     biomeIndex < 5;
                     ++biomeIndex) {
                    auto& pool = pools[kind][biomeIndex];

                    const auto biomeKind =
                        static_cast<biome::BiomeKind>(
                            biomeIndex
                        );

                    for (std::size_t assetIndex = 0;
                         assetIndex < catalog.entries.size();
                         ++assetIndex) {
                        const auto& entry =
                            catalog.entries[assetIndex];

                        if (!entry.enabled ||
                            static_cast<std::size_t>(entry.kind) !=
                                kind ||
                            !std::isfinite(entry.weight) ||
                            entry.weight <= 0.0f ||
                            !matches_habitat(
                                entry.habitat,
                                biomeKind
                            )) {
                            continue;
                            }

                        pool.add(
                            static_cast<std::uint32_t>(
                                assetIndex
                            ),
                            entry.weight
                        );
                         }
                     }
            }

            return pools;
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

    bool VegetationLayer::valid() const noexcept {
        const std::size_t count =
            static_cast<std::size_t>(samplesX) * samplesZ;

        if (samplesX < 2 ||
            samplesZ < 2 ||
            !std::isfinite(originX) ||
            !std::isfinite(originZ) ||
            !std::isfinite(spacingMeters) ||
            spacingMeters <= 0.0f ||
            grassMask.size() != count ||
            bushMask.size() != count ||
            treeMask.size() != count ||
            instances.size() > settings.maxInstances) {
            return false;
            }

        for (const auto& instance : instances) {
            if (instance.assetIndex >= assetPaths.size()) {
                return false;
            }
        }

        return true;
    }

    const std::string& VegetationLayer::asset_path(const VegetationInstance& instance) const {
        return assetPaths.at(instance.assetIndex);
    }

    VegetationLayer generate_vegetation(
        const terrain::Heightfield& terrain,
        const biome::BiomeLayer& biomes,
        const biome::HydrologyLayer* hydrology,
        const world_generator::MapLayout* layout,
        const VegetationSettings& settings,
        const VegetationAssetCatalog& catalog
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

        if (catalog.entries.empty() || catalog.entries.size() >
        (std::numeric_limits<std::uint32_t>::max)()) {
            throw std::invalid_argument(
                "Vegetation asset catalog is empty or too large"
            );
        }

        const AssetPools assetPools =
            build_asset_pools(catalog);

        bool hasAnyAssets = false;

        for (const auto& kindPools : assetPools) {
            for (const auto& pool : kindPools) {
                if (!pool.empty()) {
                    hasAnyAssets = true;
                    break;
                }
            }
        }

        if (!hasAnyAssets) {
            throw std::invalid_argument(
                "No enabled vegetation assets for generation"
            );
        }

        VegetationLayer layer;

        layer.samplesX = terrain.samplesX;
        layer.samplesZ = terrain.samplesZ;
        layer.originX = terrain.originX;
        layer.originZ = terrain.originZ;
        layer.spacingMeters = terrain.spacingMeters;
        layer.settings = settings;

        layer.assetPaths.reserve(
            catalog.entries.size()
        );

        for (const auto& entry : catalog.entries) {
            layer.assetPaths.push_back(
                entry.relativePath
            );
        }

        const std::size_t sampleCount =
            terrain.heights.size();

        const RoadIndex roadIndex(
            terrain,
            layout,
            settings.roadClearanceMeters
        );

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
                    worldX, worldZ, layout, settings, roadIndex
                )) {
                    densities = {};
                }

                for (std::size_t i = 0; i < kinds.size(); ++i) {
                    if (!allows_slope(kinds[i], slope)) {
                        densities[i] = 0.0f;
                    }
                }

                const std::size_t biomeIndex = static_cast<std::size_t>(biomeKind);

                for (std::size_t i = 0; i < 3; ++i) {
                    if (assetPools[i][biomeIndex].empty()) {
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
                        px, pz, layout, settings, roadIndex
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
                        settings.decorationSeed, x, z, salt + 4u
                    );

                    VegetationInstance instance;

                    instance.x = px;
                    instance.y = terrain.sample_height(px, pz);
                    instance.z = pz;

                    instance.yawRadians = twoPi * random01(
                        settings.decorationSeed, x, z, salt + 5u
                    );

                    instance.scale =
                        0.78f + random01(
                            settings.decorationSeed, x, z, salt + 6u
                        ) * 0.44f;

                    instance.kind = kind;

                    instance.assetIndex =
                        assetPools[i][biomeIndex].pick(
                            variation
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
