#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/World/TerrainHeightData.h"

#include <string>

namespace core::world
{
    class TerrainMeshBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const TerrainHeightData& heightData,
            assets::MeshData& output,
            std::string& error) const;
    };
}