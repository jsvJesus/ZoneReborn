#pragma once

#include "Core/Assets/TextureAnimation.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets
{
    class TextureAnimationLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view resourceReference,
            TextureAnimation& output,
            std::string& error) const;
    };
}