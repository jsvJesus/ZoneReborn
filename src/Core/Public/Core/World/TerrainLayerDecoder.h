#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/TerrainLayerData.h"

#include <string>
#include <string_view>
#include <vector>

namespace core::world
{
    class TerrainLayerDecoder final
    {
    public:
        [[nodiscard]]
        bool Decode(
            const resources::ResourceFileSystem& resources,
            std::string_view cdataLogicalPath,
            std::vector<TerrainLayerData>& output,
            std::string& error) const;
    };
}