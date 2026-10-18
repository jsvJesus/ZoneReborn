#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace engine::assets {

    struct PackedNode {
        std::string name;
        std::uint8_t type = 0;

        std::vector<std::uint8_t> data;
        std::vector<PackedNode> children;

        [[nodiscard]] const PackedNode* find(
            std::string_view childName
        ) const noexcept;

        [[nodiscard]] std::string text() const;
        [[nodiscard]] std::uint32_t unsigned_value() const;
        [[nodiscard]] bool boolean() const;
    };

    [[nodiscard]] PackedNode load_packed_section(
        const std::filesystem::path& path
    );

}
