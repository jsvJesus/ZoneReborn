#pragma once

#include "Core/Math/Vector3.h"

#include <array>
#include <cstddef>

namespace core::math
{
    struct Transform3x4 final
    {
        std::array<float, 12> values
        {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f
        };

        [[nodiscard]]
        float& operator[](
            const std::size_t index) noexcept
        {
            return values[index];
        }

        [[nodiscard]]
        const float& operator[](
            const std::size_t index) const noexcept
        {
            return values[index];
        }

        [[nodiscard]]
        Vector3 Translation() const noexcept
        {
            return
            {
                values[9],
                values[10],
                values[11]
            };
        }
    };
}