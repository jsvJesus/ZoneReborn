#pragma once

#include "Core/Resources/ResourceType.h"

#include <filesystem>
#include <string_view>

namespace core::resources
{
    class ResourceFormatRegistry final
    {
    public:
        [[nodiscard]]
        static ResourceType Detect(
            const std::filesystem::path& path);

        [[nodiscard]]
        static std::string_view Name(
            ResourceType type) noexcept;
    };
}