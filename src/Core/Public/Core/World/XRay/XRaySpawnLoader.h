#pragma once

#include "Core/World/XRay/XRaySpawnData.h"
#include <filesystem>

namespace core::world::xray
{
    class SpawnLoader final
    {
    public:
        bool Load(const std::filesystem::path& file, SpawnData& output,
            std::string& error) const;
    };
}
