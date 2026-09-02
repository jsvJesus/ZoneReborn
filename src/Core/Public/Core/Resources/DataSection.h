#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace core::resources
{
    class DataSection final
    {
    public:
        using FloatArray = std::vector<float>;
        using BinaryData = std::vector<std::byte>;

        using Value = std::variant<
            std::monostate,
            std::string,
            std::int64_t,
            FloatArray,
            bool,
            BinaryData>;

        std::string name;
        Value value;
        std::vector<DataSection> children;

        [[nodiscard]]
        const DataSection* FindChild(
            std::string_view childName) const noexcept;

        [[nodiscard]]
        std::vector<const DataSection*> FindChildren(
            std::string_view childName) const;

        [[nodiscard]]
        const std::string* AsString() const noexcept;

        [[nodiscard]]
        const std::int64_t* AsInteger() const noexcept;

        [[nodiscard]]
        const FloatArray* AsFloats() const noexcept;

        [[nodiscard]]
        const bool* AsBoolean() const noexcept;

        [[nodiscard]]
        const BinaryData* AsBinary() const noexcept;

        [[nodiscard]]
        bool TryGetFloat(
            float& output) const noexcept;
    };
}