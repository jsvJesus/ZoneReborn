#pragma once

#include "Core/Resources/DataSection.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace core::resources
{
    class PackedSectionReader final
    {
    public:
        static constexpr std::uint32_t Signature =
            0x62A14E45u;

        [[nodiscard]]
        bool Read(
            std::span<const std::byte> data,
            DataSection& output,
            std::string& error) const;

        [[nodiscard]]
        static bool HasSignature(
            std::span<const std::byte> data) noexcept;
    };
}