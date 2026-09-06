#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Flora/FloraConfig.h"

#include <string>

namespace core::world::flora
{
    class FloraConfigLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            FloraConfig& output,
            std::string& error) const;
    };
}