#pragma once

#include <cstdint>

namespace world_editor::terrain::noise {

    [[nodiscard]] double value(
        double x,
        double z,
        std::uint32_t seed
    ) noexcept;

    [[nodiscard]] double fractal(
        double x,
        double z,
        std::uint32_t seed,
        std::uint32_t octaves
    ) noexcept;

}
