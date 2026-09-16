#pragma once

#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Sky/SkyDefinition.h"

#include <string>
#include <string_view>

namespace core::world::sky
{
    class SkyLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view resourceReference,
            SkyDefinition& output,
            std::string& error) const;

    private:
        resources::PackedSectionReader
            reader_;
    };
}