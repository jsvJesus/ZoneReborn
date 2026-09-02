#pragma once

#include "Core/Assets/TextureResource.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string_view>

namespace core::assets
{
    class TextureResolver final
    {
    public:
        [[nodiscard]]
        bool Resolve(
            const resources::ResourceFileSystem& resources,
            std::string_view textureReference,
            TextureResource& output) const;
    };
}