#pragma once

#include <cstdint>
#include <string>

namespace studio
{
    class EditorScene final
    {
    public:
        EditorScene();

        void New();

        [[nodiscard]]
        const std::string& Name() const noexcept;

        [[nodiscard]]
        std::uint64_t Revision() const noexcept;

        [[nodiscard]]
        bool IsEmpty() const noexcept;

        [[nodiscard]]
        bool IsDirty() const noexcept;

    private:
        std::string name_;

        std::uint64_t revision_ = 0;

        bool empty_ = true;
        bool dirty_ = false;
    };
}