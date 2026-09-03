#include "Core/World/TerrainLoader.h"

#include "Core/World/TerrainHeightDecoder.h"
#include "Core/World/TerrainMeshBuilder.h"

#include <string>
#include <utility>

namespace core::world
{
    bool TerrainLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view cdataLogicalPath,
        TerrainAsset& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        TerrainAsset terrain;

        terrain.cdataLogicalPath =
            std::string(
                cdataLogicalPath);

        TerrainHeightDecoder
            heightDecoder;

        if (!heightDecoder.Decode(
                resources,
                cdataLogicalPath,
                terrain.heightData,
                error))
        {
            return false;
        }

        TerrainMeshBuilder
            meshBuilder;

        if (!meshBuilder.Build(
                terrain.heightData,
                terrain.mesh,
                error))
        {
            return false;
        }

        output =
            std::move(terrain);

        return true;
    }
}