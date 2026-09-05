#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace core::compression
{
    class ZlibDecoder final
    {
    public:
        [[nodiscard]]
        bool Decode(
            std::span<const std::byte> compressed,
            std::size_t expectedSize,
            std::vector<std::byte>& output,
            std::string& error) const;
    };
}