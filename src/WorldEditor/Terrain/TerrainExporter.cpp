
#include "WorldEditor/Terrain/TerrainExporter.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>

namespace world_editor::terrain
{
    void save_obj(
        const Heightfield& terrain,
        const std::filesystem::path& filePath
    ) {
        if (!terrain.valid()) {
            throw std::invalid_argument(
                "Invalid heightfield"
            );
        }

        std::ofstream output(
            filePath,
            std::ios::out | std::ios::trunc
        );

        if (!output) {
            throw std::runtime_error(
                "Cannot open OBJ output file"
            );
        }

        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(4);

        for (std::uint32_t z = 0;
             z < terrain.samplesZ;
             ++z) {

            for (std::uint32_t x = 0;
                 x < terrain.samplesX;
                 ++x) {

                const double wx =
                    static_cast<double>(terrain.originX) +
                    static_cast<double>(x) *
                    terrain.spacingMeters;

                const double wz =
                    static_cast<double>(terrain.originZ) +
                    static_cast<double>(z) *
                    terrain.spacingMeters;

                output << "v "
                       << wx << ' '
                       << terrain.height_at(x, z) << ' '
                       << wz << '\n';
            }
        }

        for (std::uint32_t z = 0;
             z + 1 < terrain.samplesZ;
             ++z) {

            for (std::uint32_t x = 0;
                 x + 1 < terrain.samplesX;
                 ++x) {

                const std::size_t a =
                    static_cast<std::size_t>(z) *
                    terrain.samplesX + x + 1;

                const std::size_t b = a + 1;
                const std::size_t c =
                    a + terrain.samplesX;
                const std::size_t d = c + 1;

                output << "f "
                       << a << ' '
                       << c << ' '
                       << b << '\n';

                output << "f "
                       << b << ' '
                       << c << ' '
                       << d << '\n';
            }
        }

        if (!output) {
            throw std::runtime_error(
                "Failed to write OBJ output file"
            );
        }
    }

    void save_pgm(
        const Heightfield& terrain,
        const std::filesystem::path& filePath
    ) {
        if (!terrain.valid()) {
            throw std::invalid_argument(
                "Invalid heightfield"
            );
        }

        std::ofstream output(
            filePath,
            std::ios::binary | std::ios::trunc
        );

        if (!output) {
            throw std::runtime_error(
                "Cannot open PGM output file"
            );
        }

        const auto [minIt, maxIt] =
            std::minmax_element(
                terrain.heights.begin(),
                terrain.heights.end()
            );

        const float minHeight = *minIt;
        const float range = *maxIt - minHeight;

        output << "P5\n"
               << terrain.samplesX << ' '
               << terrain.samplesZ
               << "\n65535\n";

        for (const float height : terrain.heights) {
            const double fraction = range > 0.0f
                ? std::clamp(
                    static_cast<double>(
                        (height - minHeight) / range
                    ),
                    0.0,
                    1.0
                )
                : 0.5;

            const auto pixel =
                static_cast<std::uint16_t>(
                    std::lround(fraction * 65535.0)
                );

            output.put(
                static_cast<char>(pixel >> 8)
            );

            output.put(
                static_cast<char>(pixel & 0xFF)
            );
        }

        if (!output) {
            throw std::runtime_error(
                "Failed to write PGM output file"
            );
        }
    }
}
