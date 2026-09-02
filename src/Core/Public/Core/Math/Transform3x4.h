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

        [[nodiscard]]
        static Transform3x4 Identity() noexcept
        {
            return {};
        }

        [[nodiscard]]
        static Transform3x4 Translation(
            const float x,
            const float y,
            const float z) noexcept
        {
            Transform3x4 result;

            result.values[9] = x;
            result.values[10] = y;
            result.values[11] = z;

            return result;
        }

        [[nodiscard]]
        static Transform3x4 Multiply(
            const Transform3x4& local,
            const Transform3x4& parent) noexcept
        {
            Transform3x4 result;

            for (std::size_t row = 0;
                 row < 3;
                 ++row)
            {
                for (std::size_t column = 0;
                     column < 3;
                     ++column)
                {
                    result.values[
                        row * 3 +
                        column] =
                        local.values[
                            row * 3 + 0] *
                            parent.values[
                                0 * 3 + column] +

                        local.values[
                            row * 3 + 1] *
                            parent.values[
                                1 * 3 + column] +

                        local.values[
                            row * 3 + 2] *
                            parent.values[
                                2 * 3 + column];
                }
            }

            result.values[9] =
                local.values[9] *
                    parent.values[0] +
                local.values[10] *
                    parent.values[3] +
                local.values[11] *
                    parent.values[6] +
                parent.values[9];

            result.values[10] =
                local.values[9] *
                    parent.values[1] +
                local.values[10] *
                    parent.values[4] +
                local.values[11] *
                    parent.values[7] +
                parent.values[10];

            result.values[11] =
                local.values[9] *
                    parent.values[2] +
                local.values[10] *
                    parent.values[5] +
                local.values[11] *
                    parent.values[8] +
                parent.values[11];

            return result;
        }
    };
}