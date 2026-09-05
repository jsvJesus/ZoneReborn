#pragma once

#include "Core/Assets/SpeedTree/CTreeData.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets::speedtree
{
    class CTreeLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view sptLogicalPath,
            CTreeAsset& output,
            std::string& error) const;
    };
}