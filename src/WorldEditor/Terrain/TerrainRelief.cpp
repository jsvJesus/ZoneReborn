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
            double minimum,
            double maximum
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

        void validate_profile(const ReliefProfile& profile) {
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
                    area / 900000.0 *
                    profile.mountainCoverage + 0.5
                )),
                1,
                18
            );

            Random random(
                static_cast<std::uint64_t>(settings.seed) ^
                0xB39C74EAull
            );

            result.reserve(count);

            for (int i = 0; i < count; ++i) {
                const double x = settings.originX +
                    random_range(
                        random,
                        width * 0.16,
                        width * 0.84
                    );

                const double z = settings.originZ +
                    random_range(
                        random,
                        depth * 0.16,
                        depth * 0.84
                    );

                const double radiusBase = side * (
                    0.10 +
                    profile.mountainCoverage * 0.16
                );

                const double angle =
                    random_range(random, 0.0, 6.283185307);

                Massif massif;

                massif.x = x;
                massif.z = z;

                massif.radiusX = radiusBase *
                    random_range(random, 0.85, 1.55);

                massif.radiusZ = radiusBase *
                    random_range(random, 0.85, 1.55);

                massif.cosine = std::cos(angle);
                massif.sine = std::sin(angle);

                massif.strength =
                    random_range(random, 0.75, 1.0);

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
                55.0f, 0.0f, 0.0f, 1.5f
            };

        case ReliefPreset::Highlands:
            return {
                ReliefPreset::Highlands,
                90.0f, 130.0f, 0.40f, 1.8f
            };

        case ReliefPreset::Mountains:
            return {
                ReliefPreset::Mountains,
                70.0f, 480.0f, 0.55f, 2.4f
            };

        case ReliefPreset::Mixed:
            return {
                ReliefPreset::Mixed,
                45.0f, 260.0f, 0.30f, 2.0f
            };
        }

        return relief_preset(ReliefPreset::Original);
    }

    Heightfield generate_with_relief(
        const Settings& settings,
        const ReliefProfile& profile
    ) {
        validate_profile(profile);

        // Preserve the exact legacy Terrain generator.
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

                // Broad rolling terrain.
                const double broad = noise::fractal(
                    worldX / 540.0,
                    worldZ / 540.0,
                    seed ^ 0x732A86B1u,
                    4
                );

                const double local = noise::fractal(
                    worldX / 190.0,
                    worldZ / 190.0,
                    seed ^ 0xAC794C5Du,
                    3
                );

                const double hills = (
                    broad * 0.75 +
                    local * 0.25
                ) * profile.hillHeightMeters;

                // Separate mountain regions leave valleys
                // available for POI and road planning.
                const double envelope = mountain_envelope(
                    massifs,
                    worldX,
                    worldZ
                );

                const double ridgeNoise = noise::fractal(
                    worldX / 125.0,
                    worldZ / 125.0,
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
                    ) * 2.0
                );

                const double detail = noise::fractal(
                    worldX / 58.0,
                    worldZ / 58.0,
                    seed ^ 0xE8BA137Du,
                    3
                );

                const double mountainHeight =
                    profile.mountainHeightMeters *
                    envelope *
                    (0.30 + 0.70 * ridge) *
                    (1.0 + detail * 0.12);

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
