#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace client::character
{
    struct FaceBoneTransform final
    {
        std::string bone;
        std::string pairedBone;
        std::array<float, 3> translation{};
        std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
    };

    [[nodiscard]]
    std::vector<std::uint64_t> GenerateRandomFaceForm(
        std::mt19937& random);

    [[nodiscard]]
    bool DecodeFaceForm(
        std::span<const std::uint64_t> packed,
        std::vector<FaceBoneTransform>& output,
        std::string& error);

    [[nodiscard]]
    bool ValidateFaceForm(
        std::span<const std::uint64_t> packed) noexcept;
}
