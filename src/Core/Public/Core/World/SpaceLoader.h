#pragma once

#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/SpaceSettings.h"

#include <string>
#include <string_view>

namespace core::world
{
    class SpaceLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view spaceName,
            SpaceSettings& output,
            std::string& error) const;

    private:
        resources::PackedSectionReader reader_;
    };
}