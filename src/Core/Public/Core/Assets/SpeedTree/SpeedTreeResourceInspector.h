#pragma once

#include "Core/Assets/SpeedTree/SpeedTreeResourceInfo.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace core::assets::speedtree
{
    class SpeedTreeResourceInspector final
    {
    public:
        [[nodiscard]]
        bool Inspect(
            const resources::ResourceFileSystem& resources,
            std::string_view sptLogicalPath,
            SpeedTreeResourceInfo& output,
            std::string& error) const;
    };
}