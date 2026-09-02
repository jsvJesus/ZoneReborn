#pragma once

#include <string>
#include <string_view>

namespace core::resources
{
    class ResourcePath final
    {
    public:
        [[nodiscard]]
        static std::string Normalize(
            std::string_view path);

        [[nodiscard]]
        static std::string ToResPath(
            std::string_view path);

        [[nodiscard]]
        static bool IsValid(
            std::string_view path) noexcept;
    };
}