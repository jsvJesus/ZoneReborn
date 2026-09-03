#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::images
{
    struct RgbaImage final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::vector<std::byte> pixels;
    };
}