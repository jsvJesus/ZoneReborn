#pragma once

#include "Core/Assets/ModelAsset.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets
{
    class ModelLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view modelReference,
            ModelAsset& output,
            std::string& error) const;
    };
}