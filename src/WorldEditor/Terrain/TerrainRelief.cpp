#include "WorldEditor/Terrain/TerrainRelief.h"

#include "WorldEditor/Terrain/Noise.h"
#include "WorldEditor/Terrain/TerrainGenerator.h"
#include "WorldEditor/WorldGenerator/DeterministicRandom.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace world_editor::terrain {

    namespace {

        using Random =
            world_generator::DeterministicRandom;

        struct Massif {
            double x = 0.0;
            double z = 0.0;

            double radiusX = 1.0;
            double radiusZ = 1.0;

            double cosine = 1.0;
            double sine = 0.0;

            double strength = 1.0;
        };

        [[nodiscard]] double random01(Random& random) {
            return static_cast<double>(random.next()) /
                4294967296.0;
        }

        [[nodiscard]] double random_range(
            Random& random,
            const double minimum,
            const double maximum
        ) {
            return minimum +
                (maximum - minimum) * random01(random);
        }

        [[nodiscard]] double smoothstep(
            double x
        ) noexcept {
            x = std::clamp(x, 0.0, 1.0);
            return x * x * (3.0 - 2.0 * x);
        }

        void validate_profile(
            const ReliefProfile& profile
        ) {
            if (static_cast<std::uint32_t>(profile.preset) > 4 ||
                !std::isfinite(profile.hillHeightMeters) ||
                !std::isfinite(profile.mountainHeightMeters) ||
                !std::isfinite(profile.mountainCoverage) ||
                !std::isfinite(profile.ridgeSharpness) ||
                profile.hillHeightMeters < 0.0f ||
                profile.hillHeightMeters > 300.0f ||
                profile.mountainHeightMeters < 0.0f ||
                profile.mountainHeightMeters > 1200.0f ||
                profile.mountainCoverage < 0.0f ||
                profile.mountainCoverage > 1.0f ||
                profile.ridgeSharpness < 0.5f ||
                profile.ridgeSharpness > 5.0f) {
                throw std::invalid_argument(
                    "Invalid Terrain Relief profile"
                );
            }
        }

        [[nodiscard]] std::vector<Massif> make_massifs(
            const Settings& settings,
            const ReliefProfile& profile
        ) {
            std::vector<Massif> result;

            if (profile.mountainHeightMeters == 0.0f ||
                profile.mountainCoverage == 0.0f) {
                return result;
            }

            const double width =
                static_cast<double>(settings.samplesX - 1) *
                settings.spacingMeters;

            const double depth =
                static_cast<double>(settings.samplesZ - 1) *
                settings.spacingMeters;

            const double area = width * depth;
            const double side = std::min(width, depth);

            const int count = std::clamp(
                static_cast<int>(std::round(
                    area / 1300000.0 *
                    (0.65 + profile.mountainCoverage)
                )),
                1,
                12
            );

            Random random(
                static_cast<std::uint64_t>(settings.seed) ^
                0xB39C74EAull
            );

            result.reserve(count);

            const double innerMargin = side * 0.08;
            const double edgeBandMin = side * 0.04;
            const double edgeBandMax = side * 0.18;

            for (int i = 0; i < count; ++i) {
                const bool edgeBiased =
                    random.bounded(100) < 70;

                double x = 0.0;
                double z = 0.0;

                if (edgeBiased) {
                    switch (random.bounded(4)) {
                    case 0: // North
                        x = settings.originX + random_range(
                            random,
                            innerMargin,
                            width - innerMargin
                        );

                        z = settings.originZ + random_range(
                            random,
                            edgeBandMin,
                            edgeBandMax
                        );
                        break;

                    case 1: // East
                        x = settings.originX + random_range(
                            random,
                            width - edgeBandMax,
                            width - edgeBandMin
                        );

                        z = settings.originZ + random_range(
                            random,
                            innerMargin,
                            depth - innerMargin
                        );
                        break;

                    case 2: // South
                        x = settings.originX + random_range(
                            random,
                            innerMargin,
                            width - innerMargin
                        );

                        z = settings.originZ + random_range(
                            random,
                            depth - edgeBandMax,
                            depth - edgeBandMin
                        );
                        break;

                    default: // West
                        x = settings.originX + random_range(
                            random,
                            edgeBandMin,
                            edgeBandMax
                        );

                        z = settings.originZ + random_range(
                            random,
                            innerMargin,
                            depth - innerMargin
                        );
                        break;
                    }
                } else {
                    x = settings.originX + random_range(
                        random,
                        width * 0.15,
                        width * 0.85
                    );

                    z = settings.originZ + random_range(
                        random,
                        depth * 0.15,
                        depth * 0.85
                    );
                }

                const double radiusBase = side * (
                    0.045 +
                    profile.mountainCoverage * 0.065
                );

                const double angle =
                    random_range(random, 0.0, 6.283185307);

                Massif massif;

                massif.x = x;
                massif.z = z;

                massif.radiusX = radiusBase *
                    random_range(random, 0.75, 1.25);

                massif.radiusZ = radiusBase *
                    random_range(random, 0.75, 1.25);

                massif.cosine = std::cos(angle);
                massif.sine = std::sin(angle);

                massif.strength =
                    random_range(random, 0.72, 1.0);

                result.push_back(massif);
            }

            return result;
        }

        [[nodiscard]] double mountain_envelope(
            const std::vector<Massif>& massifs,
            const double worldX,
            const double worldZ
        ) {
            double strongest = 0.0;

            for (const Massif& massif : massifs) {
                const double dx = worldX - massif.x;
                const double dz = worldZ - massif.z;

                const double localX =
                    dx * massif.cosine +
                    dz * massif.sine;

                const double localZ =
                    -dx * massif.sine +
                    dz * massif.cosine;

                const double nx = localX / massif.radiusX;
                const double nz = localZ / massif.radiusZ;

                const double distance =
                    std::hypot(nx, nz);

                const double influence = smoothstep(
                    1.0 - distance
                ) * massif.strength;

                strongest = std::max(
                    strongest,
                    influence
                );
            }

            return strongest;
        }

    }

    ReliefProfile relief_preset(
        const ReliefPreset preset
    ) noexcept {
        switch (preset) {
        case ReliefPreset::Original:
            return {
                ReliefPreset::Original,
                0.0f, 0.0f, 0.0f, 1.5f
            };

        case ReliefPreset::RollingHills:
            return {
                ReliefPreset::RollingHills,
                42.0f, 0.0f, 0.0f, 1.5f
            };

        case ReliefPreset::Highlands:
            return {
                ReliefPreset::Highlands,
                62.0f, 95.0f, 0.28f, 1.8f
            };

        case ReliefPreset::Mountains:
            return {
                ReliefPreset::Mountains,
                48.0f, 220.0f, 0.34f, 2.2f
            };

        case ReliefPreset::Mixed:
            return {
                ReliefPreset::Mixed,
                36.0f, 145.0f, 0.22f, 1.9f
            };
        }

        return relief_preset(ReliefPreset::Original);
    }

    Heightfield generate_with_relief(
        const Settings& settings,
        const ReliefProfile& profile
    ) {
        validate_profile(profile);

        Heightfield result = generate(settings);

        if (profile.preset == ReliefPreset::Original) {
            return result;
        }

        const auto massifs = make_massifs(
            settings,
            profile
        );

        const std::uint32_t seed = settings.seed;

        for (std::uint32_t z = 0;
             z < result.samplesZ;
             ++z) {
            for (std::uint32_t x = 0;
                 x < result.samplesX;
                 ++x) {
                const double worldX =
                    result.originX +
                    static_cast<double>(x) *
                    result.spacingMeters;

                const double worldZ =
                    result.originZ +
                    static_cast<double>(z) *
                    result.spacingMeters;

                const double broad = noise::fractal(
                    worldX / 560.0,
                    worldZ / 560.0,
                    seed ^ 0x732A86B1u,
                    4
                );

                const double local = noise::fractal(
                    worldX / 210.0,
                    worldZ / 210.0,
                    seed ^ 0xAC794C5Du,
                    3
                );

                const double hills = (
                    broad * 0.72 +
                    local * 0.28
                ) * profile.hillHeightMeters;

                const double envelope = mountain_envelope(
                    massifs,
                    worldX,
                    worldZ
                );

                const double ridgeNoise = noise::fractal(
                    worldX / 145.0,
                    worldZ / 145.0,
                    seed ^ 0x59D8F0A3u,
                    5
                );

                const double ridge = std::pow(
                    std::clamp(
                        1.0 - std::abs(ridgeNoise),
                        0.0,
                        1.0
                    ),
                    static_cast<double>(
                        profile.ridgeSharpness
                    ) * 1.8
                );

                const double detail = noise::fractal(
                    worldX / 72.0,
                    worldZ / 72.0,
                    seed ^ 0xE8BA137Du,
                    3
                );

                const double mountainHeight =
                    profile.mountainHeightMeters *
                    envelope *
                    (0.34 + 0.66 * ridge) *
                    (1.0 + detail * 0.10);

                const std::size_t index =
                    static_cast<std::size_t>(z) *
                    result.samplesX + x;

                const double height =
                    static_cast<double>(
                        result.heights[index]
                    ) + hills + mountainHeight;

                result.heights[index] =
                    static_cast<float>(height);
            }
        }

        return result;
    }

}
