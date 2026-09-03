#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/TerrainHeightData.h"

#include <string>
#include <string_view>

namespace core::world
{
    struct TerrainAsset final
    {
        std::string cdataLogicalPath;

        TerrainHeightData heightData;

        assets::MeshData mesh;
    };

    class TerrainLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view cdataLogicalPath,
            TerrainAsset& output,
            std::string& error) const;
    };
}