#pragma once

#include "Core/Math/Vector3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace core::world::xray
{
    struct DetailVertex final
    {
        math::Vector3 position;
        float u = 0.0f, v = 0.0f;
    };
    static_assert(sizeof(DetailVertex) == 20u);

    struct DetailModel final
    {
        std::string shader, texture;
        std::uint32_t flags = 0;
        float minimumScale = 1.0f, maximumScale = 1.0f;
        math::Vector3 minimum{}, maximum{};
        std::vector<DetailVertex> vertices;
        std::vector<std::uint16_t> indices;
    };

    // Native v3 packed slot; explicit masks avoid compiler-dependent bitfields.
    struct DetailSlot final
    {
        std::uint32_t low = 0, high = 0;
        std::array<std::uint16_t, 4> palettes{};
        float Base() const noexcept { return float(low & 0xfffu)*0.2f-200.0f; }
        float Height() const noexcept { return float((low >> 12u) & 0xffu)*0.1f; }
        std::uint32_t Model(std::size_t part) const noexcept
        {
            switch (part)
            {
            case 0: return (low >> 20u) & 63u;
            case 1: return low >> 26u;
            case 2: return high & 63u;
            default: return (high >> 6u) & 63u;
            }
        }
        float Sun() const noexcept { return float((high >> 12u) & 15u)/15.0f; }
        float Hemi() const noexcept { return float((high >> 16u) & 15u)/15.0f; }
        float Density(std::size_t part, float x, float z) const noexcept
        {
            const auto p = palettes[part];
            const float a = float(p & 15u)*(1.0f-x)+float((p >> 4u) & 15u)*x;
            const float b = float((p >> 8u) & 15u)*(1.0f-x)+float(p >> 12u)*x;
            return (a*(1.0f-z)+b*z)/15.0f;
        }
    };
    static_assert(sizeof(DetailSlot) == 16u);

    struct DetailStatistics final
    {
        std::uint32_t version = 0, sizeX = 0, sizeZ = 0;
        std::int32_t offsetX = 0, offsetZ = 0;
        std::size_t models = 0;
        std::uint64_t slots = 0, populatedSlots = 0, mappedBytes = 0;
    };

    class DetailsData final
    {
    public:
        DetailsData();
        ~DetailsData();
        DetailsData(const DetailsData&) = delete;
        DetailsData& operator=(const DetailsData&) = delete;
        bool Load(const std::filesystem::path& file, std::string& error);
        DetailStatistics Statistics() const noexcept;
        const std::vector<DetailModel>& Models() const noexcept;
        bool ReadSlot(std::int64_t x, std::int64_t z, DetailSlot& slot) const noexcept;
    private:
        struct State;
        std::unique_ptr<State> state_;
    };
}
