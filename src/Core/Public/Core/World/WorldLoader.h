#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/WorldScene.h"

#include <string>
#include <string_view>

namespace core::world
{
    class WorldLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view spaceName,
            WorldScene& output,
            std::string& error) const;
    };
}