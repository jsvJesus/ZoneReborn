#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace core::resources
{
    class ZipArchiveReader final
    {
    public:
        [[nodiscard]]
        bool ExtractStored(
            std::span<const std::byte> archive,
            std::string_view entryName,
            std::vector<std::byte>& output,
            std::string& error) const;
    };
}