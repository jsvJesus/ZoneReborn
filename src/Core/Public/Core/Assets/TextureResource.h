#pragma once

#include <string>

namespace core::assets
{
    struct TextureResource final
    {
        std::string sourceReference;
        std::string sourceLogicalPath;

        std::string logicalPath;

        bool exists = false;
    };
}