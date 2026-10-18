#include "WorldEditor/WorldGenerator/MapLayoutGenerator.h"

#include "WorldEditor/Road/RoadRouting.h"
#include "WorldEditor/WorldGenerator/DeterministicRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace world_editor::world_generator {

    namespace {

        constexpr float exitStepMeters = 16.0f;
        constexpr float exitEndMarginMeters = 64.0f;
        constexpr float exitSeparationMeters = 128.0f;
        constexpr float exitMaxSlopeDegrees = 20.0f;
        constexpr std::uint32_t exitAttempts = 2048;

        [[nodiscard]] bool suitable_exit(
            const MapExit& exit,
            const terrain::Heightfield& heightfield,
            const settlement::WaterPredicate& isWater
        ) {
            float dx = 0.0f;
            float dz = 0.0f;

            switch (exit.side) {
            case BoundarySide::North:
                dz = 1.0f;
                break;
            case BoundarySide::East:
                dx = -1.0f;
                break;
            case BoundarySide::South:
                dz = -1.0f;
                break;
            case BoundarySide::West:
                dx = 1.0f;
                break;
            }

            for (float distance = 0.0f;
                 distance <= 32.0f;
                 distance += 16.0f) {

                const float x = exit.x + dx * distance;
                const float z = exit.z + dz * distance;

                if (isWater && isWater(x, z)) {
                    return false;
                }

                const float slope =
                    heightfield.slope_degrees(x, z);

                if (!std::isfinite(slope) ||
                    slope > exitMaxSlopeDegrees) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::vector<MapExit> place_exits(
            const MapGenerationSettings& settings,
            const terrain::Heightfield& heightfield,
            const settlement::WaterPredicate& isWater
        ) {
            DeterministicRandom random(
                settings.seeds.location ^ 0x6A09E667u
            );

            const float mapSize = static_cast<float>(
                map_size_meters(settings.mapSize)
            );

            const auto firstCell = static_cast<std::uint32_t>(
                std::ceil(
                    exitEndMarginMeters / exitStepMeters
                )
            );

            const auto lastCell = static_cast<std::uint32_t>(
                std::floor(
                    (mapSize - exitEndMarginMeters) /
                    exitStepMeters
                )
            );

            const std::uint32_t cellCount =
                lastCell - firstCell + 1;

            const std::uint32_t firstSide = random.bounded(4);

            std::vector<MapExit> result;
            result.reserve(settings.exitCount);

            for (std::uint32_t id = 0;
                 id < settings.exitCount;
                 ++id) {

                bool placed = false;

                for (std::uint32_t attempt = 0;
                     attempt < exitAttempts;
                     ++attempt) {

                    const std::uint32_t stride =
                        settings.exitCount == 2 ? 2u : 1u;

                    const auto side = static_cast<BoundarySide>(
                        (firstSide + id * stride + attempt % 4u) % 4u
                    );

                    const float along = static_cast<float>(
                        firstCell + random.bounded(cellCount)
                    ) * exitStepMeters;

                    MapExit exit;
                    exit.id = id;
                    exit.side = side;

                    switch (side) {
                    case BoundarySide::North:
                        exit.x = heightfield.originX + along;
                        exit.z = heightfield.originZ;
                        break;

                    case BoundarySide::East:
                        exit.x = heightfield.originX + mapSize;
                        exit.z = heightfield.originZ + along;
                        break;

                    case BoundarySide::South:
                        exit.x = heightfield.originX + along;
                        exit.z = heightfield.originZ + mapSize;
                        break;

                    case BoundarySide::West:
                        exit.x = heightfield.originX;
                        exit.z = heightfield.originZ + along;
                        break;
                    }

                    bool occupied = false;

                    for (const MapExit& existing : result) {
                        const float dx = exit.x - existing.x;
                        const float dz = exit.z - existing.z;

                        if (dx * dx + dz * dz <
                            exitSeparationMeters *
                            exitSeparationMeters) {
                            occupied = true;
                            break;
                        }
                    }

                    if (occupied || !suitable_exit(
                        exit,
                        heightfield,
                        isWater
                    )) {
                        continue;
                    }

                    exit.y = heightfield.sample_height(
                        exit.x,
                        exit.z
                    );

                    result.push_back(exit);
                    placed = true;
                    break;
                }

                if (!placed) {
                    throw std::runtime_error(
                        "Cannot place all map exits "
                        "on suitable boundaries"
                    );
                }
            }

            return result;
        }

    }

    void validate_configuration(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield
    ) {
        const std::uint32_t side =
            map_size_meters(settings.mapSize);

        if (side != 512 &&
            side != 1024 &&
            side != 2048 &&
            side != 4096) {
            throw std::invalid_argument(
                "Unsupported map size"
            );
        }

        if (settings.poiCount == 0 ||
            settings.poiCount > 64 ||
            settings.exitCount == 0 ||
            settings.exitCount > 16 ||
            !std::isfinite(settings.poiSeparationMeters) ||
            !std::isfinite(settings.borderClearanceMeters) ||
            settings.poiSeparationMeters < 0.0f ||
            settings.borderClearanceMeters < 0.0f ||
            settings.borderClearanceMeters >= side * 0.5f) {
            throw std::invalid_argument(
                "Invalid POI or exit configuration"
            );
        }

        if (!heightfield.valid() ||
            heightfield.samplesX != side / 8 + 1 ||
            heightfield.samplesZ != side / 8 + 1 ||
            std::abs(heightfield.spacingMeters - 8.0f) >
                0.0001f) {
            throw std::invalid_argument(
                "Terrain dimensions must match "
                "the map size at 8 m spacing"
            );
        }

        float smallestArea = 0.0f;

        constexpr std::array catalog = {
            settlement::POIType::Checkpoint,
            settlement::POIType::Farm,
            settlement::POIType::Village,
            settlement::POIType::Industrial,
            settlement::POIType::Town
        };

        const auto& allowed = settings.allowedTypes;

        for (const auto type : allowed.empty()
            ? std::vector<settlement::POIType>(
                catalog.begin(),
                catalog.end()
            )
            : allowed) {

            const auto spec = settlement::specification(type);

            const float room = static_cast<float>(side) -
                2.0f * settings.borderClearanceMeters;

            if (side < spec.minimumMapMeters ||
                spec.widthMeters > room ||
                spec.depthMeters > room) {
                continue;
            }

            const float area =
                spec.widthMeters * spec.depthMeters;

            if (smallestArea == 0.0f ||
                area < smallestArea) {
                smallestArea = area;
            }
        }

        if (smallestArea == 0.0f) {
            throw std::invalid_argument(
                "No allowed POI types fit this map"
            );
        }

        const double mapArea =
            static_cast<double>(side) * side;

        if (static_cast<double>(settings.poiCount) *
            smallestArea > mapArea) {
            throw std::invalid_argument(
                "POI footprints exceed map capacity"
            );
        }
    }

    MapLayout generate_layout(
        const MapGenerationSettings& settings,
        const terrain::Heightfield& heightfield,
        const biome::HydrologyLayer* water
    ) {
        validate_configuration(settings, heightfield);

        const settlement::WaterPredicate isWater =
            [water](float x, float z) {
                return water && water->blocks_poi(x, z);
            };

        MapLayout result;
        result.settings = settings;

        result.pois = settlement::place_pois(
            settings,
            heightfield,
            isWater
        );

        result.exits = place_exits(
            settings,
            heightfield,
            isWater
        );

        result.roads = road::build_graph(
            result.pois,
            result.exits,
            settings.seeds.road
        );

        road::route_graph(
            result.roads,
            heightfield,
            water,
            result.pois
        );

        if (result.pois.size() != settings.poiCount ||
            result.exits.size() != settings.exitCount ||
            !road::is_connected(result.roads)) {
            throw std::runtime_error(
                "Generated map layout is incomplete"
            );
        }

        return result;
    }

}
