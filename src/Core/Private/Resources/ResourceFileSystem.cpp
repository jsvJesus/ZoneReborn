#include "Core/Resources/ResourceFileSystem.h"

#include <fstream>
#include <limits>
#include <system_error>

namespace
{
    char ToLowerAscii(const char value) noexcept
    {
        if (value >= 'A' && value <= 'Z')
        {
            return static_cast<char>(value - 'A' + 'a');
        }

        return value;
    }
}

namespace core::resources
{
    bool ResourceFileSystem::Initialize(
        const std::filesystem::path& packsRoot)
    {
        Shutdown();

        std::error_code error;

        const std::filesystem::path absoluteRoot =
            std::filesystem::absolute(packsRoot, error);

        if (error)
        {
            return false;
        }

        const std::filesystem::path resRoot =
            absoluteRoot / "res";

        const std::filesystem::path sysRoot =
            absoluteRoot / "sys";

        if (!std::filesystem::is_directory(resRoot, error))
        {
            return false;
        }

        error.clear();

        if (!std::filesystem::is_directory(sysRoot, error))
        {
            return false;
        }

        packsRoot_ = absoluteRoot;

        if (!IndexMount("res", resRoot))
        {
            Shutdown();
            return false;
        }

        if (!IndexMount("sys", sysRoot))
        {
            Shutdown();
            return false;
        }

        initialized_ = true;

        return true;
    }

    void ResourceFileSystem::Shutdown()
    {
        index_.clear();
        packsRoot_.clear();
        initialized_ = false;
    }

    bool ResourceFileSystem::IsInitialized() const noexcept
    {
        return initialized_;
    }

    bool ResourceFileSystem::Exists(
        const std::string_view logicalPath) const
    {
        return Find(logicalPath) != nullptr;
    }

    const ResourceEntry* ResourceFileSystem::Find(
        const std::string_view logicalPath) const noexcept
    {
        const std::string normalized =
            NormalizeLogicalPath(logicalPath);

        if (normalized.empty())
        {
            return nullptr;
        }

        const auto iterator = index_.find(normalized);

        if (iterator == index_.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    bool ResourceFileSystem::ReadBinary(
        const std::string_view logicalPath,
        std::vector<std::byte>& output) const
    {
        output.clear();

        const ResourceEntry* entry = Find(logicalPath);

        if (entry == nullptr)
        {
            return false;
        }

        std::ifstream stream(
            entry->physicalPath,
            std::ios::binary | std::ios::ate);

        if (!stream)
        {
            return false;
        }

        const std::streampos endPosition = stream.tellg();

        if (endPosition < 0)
        {
            return false;
        }

        const auto byteCount =
            static_cast<std::uint64_t>(endPosition);

        if (byteCount >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()))
        {
            return false;
        }

        output.resize(
            static_cast<std::size_t>(byteCount));

        stream.seekg(0, std::ios::beg);

        if (!stream)
        {
            output.clear();
            return false;
        }

        if (!output.empty())
        {
            stream.read(
                reinterpret_cast<char*>(output.data()),
                static_cast<std::streamsize>(output.size()));

            if (!stream)
            {
                output.clear();
                return false;
            }
        }

        return true;
    }

    bool ResourceFileSystem::ReadText(
        const std::string_view logicalPath,
        std::string& output) const
    {
        output.clear();

        const ResourceEntry* entry = Find(logicalPath);

        if (entry == nullptr)
        {
            return false;
        }

        std::ifstream stream(
            entry->physicalPath,
            std::ios::binary | std::ios::ate);

        if (!stream)
        {
            return false;
        }

        const std::streampos endPosition = stream.tellg();

        if (endPosition < 0)
        {
            return false;
        }

        const auto byteCount =
            static_cast<std::uint64_t>(endPosition);

        if (byteCount >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()))
        {
            return false;
        }

        output.resize(
            static_cast<std::size_t>(byteCount));

        stream.seekg(0, std::ios::beg);

        if (!stream)
        {
            output.clear();
            return false;
        }

        if (!output.empty())
        {
            stream.read(
                output.data(),
                static_cast<std::streamsize>(output.size()));

            if (!stream)
            {
                output.clear();
                return false;
            }
        }

        return true;
    }

    std::size_t ResourceFileSystem::ResourceCount() const noexcept
    {
        return index_.size();
    }

    const std::filesystem::path&
    ResourceFileSystem::PacksRoot() const noexcept
    {
        return packsRoot_;
    }

    bool ResourceFileSystem::IndexMount(
        const std::string_view mountName,
        const std::filesystem::path& directory)
    {
        std::error_code error;

        std::filesystem::recursive_directory_iterator iterator(
            directory,
            std::filesystem::directory_options::skip_permission_denied,
            error);

        const std::filesystem::recursive_directory_iterator end;

        if (error)
        {
            return false;
        }

        while (iterator != end)
        {
            const std::filesystem::directory_entry& file = *iterator;

            error.clear();

            if (file.is_regular_file(error) && !error)
            {
                const std::filesystem::path relativePath =
                    file.path().lexically_relative(directory);

                if (!relativePath.empty())
                {
                    std::string logicalPath;
                    logicalPath.reserve(
                        mountName.size() +
                        1 +
                        relativePath.generic_string().size());

                    logicalPath.append(mountName);
                    logicalPath.push_back('/');
                    logicalPath.append(
                        relativePath.generic_string());

                    const std::string normalized =
                        NormalizeLogicalPath(logicalPath);

                    if (!normalized.empty())
                    {
                        error.clear();

                        const std::uintmax_t fileSize =
                            file.file_size(error);

                        ResourceEntry entry;
                        entry.logicalPath = logicalPath;
                        entry.physicalPath = file.path();
                        entry.size = error ? 0 : fileSize;

                        index_.insert_or_assign(
                            normalized,
                            std::move(entry));
                    }
                }
            }

            error.clear();
            iterator.increment(error);

            if (error)
            {
                error.clear();
            }
        }

        return true;
    }

    std::string ResourceFileSystem::NormalizeLogicalPath(
        const std::string_view logicalPath)
    {
        if (logicalPath.empty())
        {
            return {};
        }

        std::string converted;
        converted.reserve(logicalPath.size());

        for (const char character : logicalPath)
        {
            if (character == '\\')
            {
                converted.push_back('/');
            }
            else
            {
                converted.push_back(character);
            }
        }

        const std::filesystem::path path(converted);

        std::string normalized;

        for (const std::filesystem::path& component : path)
        {
            std::string part = component.generic_string();

            if (part.empty() || part == "." || part == "/")
            {
                continue;
            }

            if (part == "..")
            {
                return {};
            }

            if (!normalized.empty())
            {
                normalized.push_back('/');
            }

            for (char& character : part)
            {
                character = ToLowerAscii(character);
            }

            normalized.append(part);
        }

        return normalized;
    }
}