
#include "WorldEditor/Terrain/TerrainGenerator.h"
#include "WorldEditor/Terrain/Noise.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace world_editor::terrain
{
    Heightfield generate(const Settings& settings) {
        if (
            settings.samplesX < 2 ||
            settings.samplesX > 2049 ||
            settings.samplesZ < 2 ||
            settings.samplesZ > 2049 ||
            !std::isfinite(settings.originX) ||
            !std::isfinite(settings.originZ) ||
            !std::isfinite(settings.spacingMeters) ||
            settings.spacingMeters <= 0.0f ||
            !std::isfinite(settings.baseHeightMeters) ||
            !std::isfinite(settings.reliefMeters) ||
            settings.reliefMeters < 0.0f ||
            !std::isfinite(settings.macroScaleMeters) ||
            !std::isfinite(settings.detailScaleMeters) ||
            settings.macroScaleMeters <
                settings.spacingMeters * 2.0f ||
            settings.detailScaleMeters <
                settings.spacingMeters * 2.0f
        ) {
            throw std::invalid_argument(
                "Invalid terrain generation settings"
            );
        }

        Heightfield result;

        result.samplesX = settings.samplesX;
        result.samplesZ = settings.samplesZ;
        result.originX = settings.originX;
        result.originZ = settings.originZ;
        result.spacingMeters = settings.spacingMeters;

        result.heights.resize(
            static_cast<std::size_t>(settings.samplesX) *
            settings.samplesZ
        );

        for (std::uint32_t z = 0;
             z < settings.samplesZ;
             ++z) {

            for (std::uint32_t x = 0;
                 x < settings.samplesX;
                 ++x) {

                const double wx =
                    static_cast<double>(settings.originX) +
                    static_cast<double>(x) *
                    settings.spacingMeters;

                const double wz =
                    static_cast<double>(settings.originZ) +
                    static_cast<double>(z) *
                    settings.spacingMeters;

                const double warpX = noise::value(
                    wx / 420.0,
                    wz / 420.0,
                    settings.seed ^ 0xC2B2AE35u
                ) * 48.0;

                const double warpZ = noise::value(
                    wx / 420.0,
                    wz / 420.0,
                    settings.seed ^ 0x27D4EB2Fu
                ) * 48.0;

                const double u = wx + warpX;
                const double v = wz + warpZ;

                const double macro = noise::fractal(
                    u / settings.macroScaleMeters,
                    v / settings.macroScaleMeters,
                    settings.seed ^ 0xA341316Cu,
                    5
                );

                const double rolling = noise::fractal(
                    u / 320.0,
                    v / 320.0,
                    settings.seed ^ 0xC8013EA4u,
                    3
                );

                const double detail = noise::fractal(
                    wx / settings.detailScaleMeters,
                    wz / settings.detailScaleMeters,
                    settings.seed ^ 0xAD90777Du,
                    4
                );

                const double basin = std::max(
                    0.0,
                    -noise::fractal(
                        wx / 750.0,
                        wz / 750.0,
                        settings.seed ^ 0x7E95761Eu,
                        3
                    )
                );

                const double height =
                    settings.baseHeightMeters +
                    settings.reliefMeters * (
                        0.78 * macro +
                        0.23 * rolling +
                        0.08 * detail -
                        0.36 * basin
                    );

                result.heights[
                    static_cast<std::size_t>(z) *
                    settings.samplesX + x
                ] = static_cast<float>(height);
            }
        }

        return result;
    }

}
