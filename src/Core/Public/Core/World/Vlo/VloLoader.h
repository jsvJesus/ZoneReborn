#pragma once

#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Vlo/VloResource.h"

#include <string>
#include <string_view>

namespace core::world::vlo
{
    class VloLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view logicalPath,
            std::string_view expectedType,
            VloResource& output,
            std::string& error) const;

    private:
        resources::PackedSectionReader reader_;
    };
}