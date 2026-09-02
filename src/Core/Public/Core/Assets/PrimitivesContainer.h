#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace core::assets
{
    struct PrimitivesSection final
    {
        std::string name;

        std::size_t offset = 0;
        std::size_t size = 0;

        std::array<std::uint32_t, 4> metadata
        {
            0,
            0,
            0,
            0
        };
    };

    class PrimitivesContainer final
    {
    public:
        std::string logicalPath;

        std::vector<std::byte> data;
        std::vector<PrimitivesSection> sections;

        [[nodiscard]]
        const PrimitivesSection* FindSection(
            std::string_view name) const noexcept;

        [[nodiscard]]
        std::span<const std::byte> SectionData(
            const PrimitivesSection& section) const noexcept;

        [[nodiscard]]
        std::span<const std::byte> SectionData(
            std::string_view name) const noexcept;
    };
}