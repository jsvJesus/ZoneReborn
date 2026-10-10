#include "Studio/LevelCatalog.h"

#include <algorithm>
#include <array>
#include <system_error>

namespace studio
{
    std::string PathToUtf8(
        const std::filesystem::path& path)
    {
        const auto value = path.u8string();

        return {
            reinterpret_cast<const char*>(value.data()),
            value.size()
        };
    }

    void LevelCatalog::Refresh(
        const std::filesystem::path& gameRoot,
        const std::filesystem::path& executableDirectory)
    {
        entries_.clear();
        roots_.clear();

        std::error_code error;

        const std::filesystem::path workingDirectory =
            std::filesystem::current_path(error);

        const std::array<std::filesystem::path, 5> candidates
        {
            gameRoot / "game" / "gamedata" / "levels",
            gameRoot / "gamedata" / "levels",
            executableDirectory.parent_path() / "gamedata" / "levels",
            executableDirectory / "gamedata" / "levels",
            workingDirectory / "gamedata" / "levels"
        };

        for (const auto& candidate : candidates)
        {
            if (candidate.empty())
                continue;

            const auto root = candidate.lexically_normal();

            error.clear();

            if (!std::filesystem::is_directory(root, error))
                continue;

            if (std::find(roots_.begin(), roots_.end(), root)
                != roots_.end())
            {
                continue;
            }

            roots_.push_back(root);

            error.clear();

            std::filesystem::directory_iterator iterator(
                root,
                std::filesystem::directory_options::skip_permission_denied,
                error);

            const std::filesystem::directory_iterator end;

            while (!error && iterator != end)
            {
                std::error_code statusError;

                if (iterator->is_directory(statusError))
                {
                    const auto directory = iterator->path();

                    const bool hasLevel =
                        std::filesystem::is_regular_file(
                            directory / "level",
                            statusError);

                    statusError.clear();

                    const bool hasGeometry =
                        std::filesystem::is_regular_file(
                            directory / "level.geom",
                            statusError);

                    if (hasLevel && hasGeometry)
                    {
                        LevelEntry entry;

                        entry.name =
                            PathToUtf8(directory.filename());

                        entry.directory =
                            directory.lexically_normal();

                        entries_.push_back(std::move(entry));
                    }
                }

                iterator.increment(error);
            }
        }

        std::sort(
            entries_.begin(),
            entries_.end(),
            [](const LevelEntry& a, const LevelEntry& b)
            {
                if (a.name != b.name)
                    return a.name < b.name;

                return a.directory < b.directory;
            });

        entries_.erase(
            std::unique(
                entries_.begin(),
                entries_.end(),
                [](const LevelEntry& a, const LevelEntry& b)
                {
                    return a.directory == b.directory;
                }),
            entries_.end());
    }

    const std::vector<LevelEntry>&
    LevelCatalog::Entries() const noexcept
    {
        return entries_;
    }

    const std::vector<std::filesystem::path>&
    LevelCatalog::Roots() const noexcept
    {
        return roots_;
    }
}