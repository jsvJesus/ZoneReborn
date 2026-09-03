#pragma once

#include "Core/Images/RgbaImage.h"

#include <cstddef>
#include <span>
#include <string>

namespace core::images
{
    class PngDecoder final
    {
    public:
        [[nodiscard]]
        bool Decode(
            std::span<const std::byte> data,
            RgbaImage& output,
            std::string& error) const;
    };
}