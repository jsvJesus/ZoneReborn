#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace core::assets::speedtree
{
    struct SpeedTreeResourceInfo final
    {
        std::string sptLogicalPath;
        std::string ctreeLogicalPath;
        std::string bspLogicalPath;

        std::size_t sptSize = 0;
        std::size_t ctreeSize = 0;
        std::size_t bspSize = 0;

        bool ctreeExists = false;
        bool bspExists = false;

        bool containsPossibleZlibStream = false;

        std::vector<std::string>
            textureReferences;

        std::string ctreeHeaderHex;
    };
}