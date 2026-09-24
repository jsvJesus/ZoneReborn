#pragma once

#include "Character/CharacterFaceState.h"

#include <cstdint>
#include <string>
#include <vector>

namespace client::character
{
    struct AppearancePart final
    {
        std::string group;

        std::int32_t itemType =
            0;
    };

    struct Profile final
    {
        std::string id;
        std::string name;

        std::vector<AppearancePart>
            appearance;

        FaceState face;
    };
}
