#pragma once

#include "Core/Assets/ModelSource.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets
{
    class ModelSourceLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view modelReference,
            ModelSource& output,
            std::string& error) const;
    };
}