#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace studio
{
    std::string PathToUtf8(
        const std::filesystem::path& path);

    struct LevelEntry final
    {
        std::string name;
        std::filesystem::path directory;
    };

    class LevelCatalog final
    {
    public:
        void Refresh(
            const std::filesystem::path& gameRoot,
            const std::filesystem::path& executableDirectory);

        [[nodiscard]]
        const std::vector<LevelEntry>& Entries() const noexcept;

        [[nodiscard]]
        const std::vector<std::filesystem::path>& Roots() const noexcept;

    private:
        std::vector<LevelEntry> entries_;
        std::vector<std::filesystem::path> roots_;
    };
}