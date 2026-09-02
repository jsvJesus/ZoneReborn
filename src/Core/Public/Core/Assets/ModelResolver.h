#pragma once

#include "Core/Assets/ModelResource.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Chunk.h"

#include <string_view>
#include <vector>

namespace core::assets
{
    class ModelResolver final
    {
    public:
        [[nodiscard]]
        bool Resolve(
            const resources::ResourceFileSystem& resources,
            std::string_view modelReference,
            ModelResource& output) const;

        [[nodiscard]]
        std::vector<ModelResource> ResolveChunkModels(
            const resources::ResourceFileSystem& resources,
            const world::Chunk& chunk) const;
    };
}