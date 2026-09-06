#pragma once

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Flora/FloraVisualAsset.h"

#include <string>
#include <string_view>

namespace core::world::flora
{
    class FloraVisualLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view visualReference,
            FloraVisualAsset& output,
            std::string& error) const;
    };
}