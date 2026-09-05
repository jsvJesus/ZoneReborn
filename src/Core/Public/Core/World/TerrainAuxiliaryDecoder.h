#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/TerrainAuxiliaryData.h"

#include <string>
#include <string_view>

namespace core::world
{
    class TerrainAuxiliaryDecoder final
    {
    public:
        [[nodiscard]]
        bool Decode(
            const resources::ResourceFileSystem& resources,
            std::string_view cdataLogicalPath,
            TerrainAuxiliaryData& output,
            std::string& error) const;
    };
}