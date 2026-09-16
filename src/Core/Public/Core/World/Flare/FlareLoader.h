#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Flare/FlareDefinition.h"

#include <string>
#include <string_view>

namespace core::world::flare
{
    class FlareLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const core::resources::ResourceFileSystem& resources,
            std::string_view resourceReference,
            FlareDefinition& output,
            std::string& error) const;
    };
}