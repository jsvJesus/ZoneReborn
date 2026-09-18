#pragma once

#include <filesystem>
#include <string>

namespace client::account
{
    class RememberedLogin final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            const std::filesystem::path& gameRoot);

        [[nodiscard]]
        std::string Load() const;

        [[nodiscard]]
        bool Save(
            const std::string& login) const;

        void Clear() const;

    private:
        std::filesystem::path filePath_;
    };
}