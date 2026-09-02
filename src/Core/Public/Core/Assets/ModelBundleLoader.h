#pragma once

#include "Core/Assets/ModelBundle.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets
{
    class ModelBundleLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view modelReference,
            ModelBundle& output,
            std::string& error) const;
    };
}