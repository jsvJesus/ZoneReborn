#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace studio
{
    struct BuildLightRecord final
    {
        std::uint16_t type = 0;
        std::uint16_t level = 0;

        std::array<float, 3> diffuse{};
        std::array<float, 3> position{};
        std::array<float, 3> direction{};

        float range = 0.0f;
        float rangeSquared = 0.0f;

        float attenuation0 = 0.0f;
        float attenuation1 = 0.0f;
        float attenuation2 = 0.0f;

        float energy = 0.0f;

        std::array<float, 9> triangle{};

        std::uint32_t unknown = 0;
    };

    struct BuildLightsData final
    {
        std::uint16_t version = 0;

        std::vector<BuildLightRecord> rgb;
        std::vector<BuildLightRecord> hemi;
        std::vector<BuildLightRecord> sun;

        std::size_t auxiliaryChunks = 0;
    };

    [[nodiscard]]
    bool LoadBuildLights(
        const std::filesystem::path& path,
        BuildLightsData& output,
        std::string& error);
}