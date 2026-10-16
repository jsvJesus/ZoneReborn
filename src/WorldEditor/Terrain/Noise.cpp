#include "WorldEditor/Terrain/Noise.h"

#include <cmath>
#include <cstdint>

namespace world_editor::terrain::noise
{
    namespace
    {
        std::uint32_t hash(
            const std::int64_t x,
            const std::int64_t z,
            const std::uint32_t seed
        ) noexcept {
            std::uint32_t h = seed ^
                (static_cast<std::uint32_t>(x) * 0x9E3779B9u) ^
                (static_cast<std::uint32_t>(z) * 0x85EBCA6Bu);

            h ^= h >> 16;
            h *= 0x7FEB352Du;

            h ^= h >> 15;
            h *= 0x846CA68Bu;

            h ^= h >> 16;

            return h;
        }

        double lattice(
            const std::int64_t x,
            const std::int64_t z,
            const std::uint32_t seed
        ) noexcept {
            return (
                static_cast<double>(hash(x, z, seed)) /
                4294967295.0
            ) * 2.0 - 1.0;
        }

        double smooth(const double t) noexcept {
            return t * t * t *
                (t * (t * 6.0 - 15.0) + 10.0);
        }

        double lerp(
            const double a,
            const double b,
            const double t
        ) noexcept {
            return a + (b - a) * t;
        }

    }

    double value(
        const double x,
        const double z,
        const std::uint32_t seed
    ) noexcept {
        const double fx = std::floor(x);
        const double fz = std::floor(z);

        const auto ix = static_cast<std::int64_t>(fx);
        const auto iz = static_cast<std::int64_t>(fz);

        const double tx = smooth(x - fx);
        const double tz = smooth(z - fz);

        const double top = lerp(
            lattice(ix, iz, seed),
            lattice(ix + 1, iz, seed),
            tx
        );

        const double bottom = lerp(
            lattice(ix, iz + 1, seed),
            lattice(ix + 1, iz + 1, seed),
            tx
        );

        return lerp(top, bottom, tz);
    }

    double fractal(
        double x,
        double z,
        const std::uint32_t seed,
        const std::uint32_t octaves
    ) noexcept {
        double amplitude = 1.0;
        double total = 0.0;
        double normalization = 0.0;

        for (std::uint32_t i = 0; i < octaves; ++i) {
            total += value(
                x,
                z,
                seed + i * 0x9E3779B9u
            ) * amplitude;

            normalization += amplitude;

            amplitude *= 0.52;
            x *= 2.0;
            z *= 2.0;
        }

        return normalization > 0.0
            ? total / normalization
            : 0.0;
    }

}
