#pragma once

#include "Core/Assets/VisualAsset.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets
{
    class VisualLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view visualReference,
            VisualAsset& output,
            std::string& error) const;
    };
}