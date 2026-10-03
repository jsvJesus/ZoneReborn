#pragma once

#include "Core/World/XRay/XRayLevelData.h"

#include <filesystem>
#include <string>

namespace core::world::xray
{
    class LevelLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const std::filesystem::path& levelDirectory,
            LevelData& output,
            std::string& error) const;
    };
}
