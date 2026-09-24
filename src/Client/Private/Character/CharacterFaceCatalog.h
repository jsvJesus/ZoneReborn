#pragma once

#include "Core/Resources/ResourceFileSystem.h"

#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace client::character
{
    struct FaceWeightedValue final
    {
        std::uint32_t value = 0;
        std::uint32_t weight = 100;
    };

    class FaceCatalog final
    {
    public:
        [[nodiscard]] bool Load(
            const core::resources::ResourceFileSystem& resources,
            std::string& error);
        void Clear();
        [[nodiscard]] bool IsStyleAllowed(
            std::string_view group,
            std::int32_t value) const noexcept;
        [[nodiscard]] bool IsColourAllowed(
            std::string_view group,
            std::uint32_t value) const noexcept;
        [[nodiscard]] std::uint32_t RandomWeighted(
            std::string_view group,
            std::mt19937& random) const;
        [[nodiscard]] const std::vector<std::int32_t>& Styles(
            std::string_view group) const noexcept;
        [[nodiscard]] std::size_t GroupCount() const noexcept;

    private:
        std::unordered_map<std::string, std::vector<FaceWeightedValue>> values_;
        std::unordered_map<std::string, std::vector<std::int32_t>> styles_;
    };
}
