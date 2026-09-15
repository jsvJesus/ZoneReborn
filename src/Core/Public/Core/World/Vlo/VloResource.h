#pragma once

#include "Core/World/Water/WaterDefinition.h"

#include <cstdint>
#include <optional>
#include <string>

namespace core::world::vlo
{
    enum class VloType : std::uint8_t
    {
        Unknown = 0,
        Water
    };

    struct VloResource final
    {
        std::string logicalPath;
        std::string typeName;

        VloType type = VloType::Unknown;

        bool hidden = false;
        bool frozen = false;

        std::optional<water::WaterDefinition>
            water;
    };
}