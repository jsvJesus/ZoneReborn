#include "WorldEditor/Settlement/POIPlanner.h"

#include "WorldEditor/WorldGenerator/DeterministicRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace world_editor::settlement {

    namespace {

        constexpr float placementStepMeters = 16.0f;
        constexpr float inspectionStepMeters = 16.0f;
        constexpr std::uint32_t attemptsPerType = 1024;

        [[nodiscard]] std::vector<POIType> eligible_types(
            const world_generator::MapGenerationSettings& settings
        ) {
            constexpr std::array defaults = {
                POIType::Checkpoint,
                POIType::Farm,
                POIType::Village,
                POIType::Industrial,
                POIType::Town
            };

            const std::vector<POIType> types =
                settings.allowedTypes.empty()
                    ? std::vector<POIType>(
                        defaults.begin(),
                        defaults.end()
                    )
                    : settings.allowedTypes;

            std::vector<POIType> result;

            const float mapSize = static_cast<float>(
                world_generator::map_size_meters(settings.mapSize)
            );

            for (const POIType type : types) {
                const POISpecification spec = specification(type);

                const float available =
                    mapSize - 2.0f * settings.borderClearanceMeters;

                if (mapSize >= spec.minimumMapMeters &&
                    spec.widthMeters <= available &&
                    spec.depthMeters <= available &&
                    std::find(result.begin(), result.end(), type) ==
                        result.end()) {
                    result.push_back(type);
                }
            }

            return result;
        }

        [[nodiscard]] bool overlaps(
            const POIPlacement& a,
            const POIPlacement& b,
            const float clearance
        ) noexcept {
            return std::abs(a.centerX - b.centerX) <
                    (a.widthMeters + b.widthMeters) * 0.5f +
                    clearance &&
                std::abs(a.centerZ - b.centerZ) <
                    (a.depthMeters + b.depthMeters) * 0.5f +
                    clearance;
        }

        [[nodiscard]] bool terrain_allows(
            const POIPlacement& candidate,
            const POISpecification& spec,
            const terrain::Heightfield& heightfield,
            const WaterPredicate& isWater
        ) {
            const auto stepsX = static_cast<std::uint32_t>(
                std::ceil(
                    spec.widthMeters / inspectionStepMeters
                )
            );

            const auto stepsZ = static_cast<std::uint32_t>(
                std::ceil(
                    spec.depthMeters / inspectionStepMeters
                )
            );

            const float minX =
                candidate.centerX - spec.widthMeters * 0.5f;

            const float minZ =
                candidate.centerZ - spec.depthMeters * 0.5f;

            for (std::uint32_t z = 0; z <= stepsZ; ++z) {
                const float worldZ = minZ +
                    spec.depthMeters * static_cast<float>(z) /
                    static_cast<float>(stepsZ);

                for (std::uint32_t x = 0; x <= stepsX; ++x) {
                    const float worldX = minX +
                        spec.widthMeters * static_cast<float>(x) /
                        static_cast<float>(stepsX);

                    if (isWater && isWater(worldX, worldZ)) {
                        return false;
                    }

                    const float slope =
                        heightfield.slope_degrees(worldX, worldZ);

                    if (!std::isfinite(slope) ||
                        slope > spec.maxSlopeDegrees) {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]] bool try_place(
            const POIType type,
            const world_generator::MapGenerationSettings& settings,
            const terrain::Heightfield& heightfield,
            const WaterPredicate& isWater,
            world_generator::DeterministicRandom& random,
            const std::vector<POIPlacement>& existing,
            POIPlacement& output
        ) {
            const POISpecification spec = specification(type);

            const float mapSize = static_cast<float>(
                world_generator::map_size_meters(settings.mapSize)
            );

            const float minimumX =
                settings.borderClearanceMeters +
                spec.widthMeters * 0.5f;

            const float minimumZ =
                settings.borderClearanceMeters +
                spec.depthMeters * 0.5f;

            const float maximumX = mapSize - minimumX;
            const float maximumZ = mapSize - minimumZ;

            const auto gridMinX = static_cast<std::int32_t>(
                std::ceil(minimumX / placementStepMeters)
            );

            const auto gridMinZ = static_cast<std::int32_t>(
                std::ceil(minimumZ / placementStepMeters)
            );

            const auto gridMaxX = static_cast<std::int32_t>(
                std::floor(maximumX / placementStepMeters)
            );

            const auto gridMaxZ = static_cast<std::int32_t>(
                std::floor(maximumZ / placementStepMeters)
            );

            if (gridMaxX < gridMinX || gridMaxZ < gridMinZ) {
                return false;
            }

            const auto columns = static_cast<std::uint32_t>(
                gridMaxX - gridMinX + 1
            );

            const auto rows = static_cast<std::uint32_t>(
                gridMaxZ - gridMinZ + 1
            );

            for (std::uint32_t attempt = 0;
                 attempt < attemptsPerType;
                 ++attempt) {

                POIPlacement candidate;

                candidate.id =
                    static_cast<std::uint32_t>(existing.size());

                candidate.type = type;
                candidate.widthMeters = spec.widthMeters;
                candidate.depthMeters = spec.depthMeters;

                candidate.centerX = heightfield.originX +
                    static_cast<float>(
                        gridMinX + random.bounded(columns)
                    ) * placementStepMeters;

                candidate.centerZ = heightfield.originZ +
                    static_cast<float>(
                        gridMinZ + random.bounded(rows)
                    ) * placementStepMeters;

                bool occupied = false;

                for (const POIPlacement& placed : existing) {
                    if (overlaps(
                        candidate,
                        placed,
                        settings.poiSeparationMeters
                    )) {
                        occupied = true;
                        break;
                    }
                }

                if (occupied || !terrain_allows(
                    candidate,
                    spec,
                    heightfield,
                    isWater
                )) {
                    continue;
                }

                candidate.centerY = heightfield.sample_height(
                    candidate.centerX,
                    candidate.centerZ
                );

                output = candidate;
                return true;
            }

            return false;
        }

    }

    std::vector<POIPlacement> place_pois(
        const world_generator::MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield,
        const WaterPredicate& isWater
    ) {
        const auto types = eligible_types(settings);

        if (types.empty()) {
            throw std::invalid_argument(
                "No POI types fit the selected map size "
                "and border clearance"
            );
        }

        world_generator::DeterministicRandom random(
            settings.seeds.location
        );

        std::vector<POIPlacement> placements;
        placements.reserve(settings.poiCount);

        for (std::uint32_t index = 0;
             index < settings.poiCount;
             ++index) {

            const auto first = random.bounded(
                static_cast<std::uint32_t>(types.size())
            );

            bool placed = false;

            for (std::size_t offset = 0;
                 offset < types.size();
                 ++offset) {

                const POIType type = types[
                    (static_cast<std::size_t>(first) + offset) %
                    types.size()
                ];

                POIPlacement candidate;

                if (try_place(
                    type,
                    settings,
                    heightfield,
                    isWater,
                    random,
                    placements,
                    candidate
                )) {
                    placements.push_back(candidate);
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                throw std::runtime_error(
                    "Insufficient suitable terrain for POI " +
                    std::to_string(index + 1) + " of " +
                    std::to_string(settings.poiCount) +
                    "; reduce POI count, footprints or separation"
                );
            }
        }

        return placements;
    }

}
