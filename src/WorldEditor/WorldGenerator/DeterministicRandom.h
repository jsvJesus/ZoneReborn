#pragma once

#include <cstdint>
#include <stdexcept>

namespace world_editor::world_generator {

    class DeterministicRandom final {
    public:
        explicit DeterministicRandom(const std::uint64_t seed)
            : state_(seed) {
        }

        [[nodiscard]] std::uint32_t next() noexcept {
            state_ += 0x9E3779B97F4A7C15ull;

            std::uint64_t value = state_;
            value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
            value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
            return static_cast<std::uint32_t>(
                (value ^ (value >> 31)) >> 32
            );
        }

        [[nodiscard]] std::uint32_t bounded(
            const std::uint32_t bound
        ) {
            if (bound == 0) {
                throw std::invalid_argument(
                    "Random bound must be positive"
                );
            }

            const std::uint32_t threshold =
                (std::uint32_t{0} - bound) % bound;

            for (;;) {
                const std::uint32_t value = next();

                if (value >= threshold) {
                    return value % bound;
                }
            }
        }

    private:
        std::uint64_t state_ = 0;
    };

}
