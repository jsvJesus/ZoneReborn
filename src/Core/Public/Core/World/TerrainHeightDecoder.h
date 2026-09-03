#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/TerrainHeightData.h"

#include <string>
#include <string_view>

namespace core::world
{
    class TerrainHeightDecoder final
    {
    public:
        [[nodiscard]]
        bool Decode(
            const resources::ResourceFileSystem& resources,
            std::string_view cdataLogicalPath,
            TerrainHeightData& output,
            std::string& error) const;
    };
}