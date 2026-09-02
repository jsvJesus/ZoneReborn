#pragma once

#include <cstdint>
#include <string>

namespace core::assets
{
    struct ModelResource final
    {
        std::string sourceReference;
        std::string logicalPath;

        std::uintmax_t fileSize = 0;

        bool exists = false;
    };
}