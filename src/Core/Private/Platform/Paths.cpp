#include "Core/Platform/Paths.h"

#include <Windows.h>

#include <system_error>
#include <vector>

namespace
{
    std::filesystem::path FindRootFrom(std::filesystem::path current)
    {
        if (current.empty())
        {
            return {};
        }

        std::error_code error;

        current = std::filesystem::absolute(current, error);

        if (error)
        {
            return {};
        }

        while (!current.empty())
        {
            error.clear();

            const bool hasRes = std::filesystem::is_directory(
                current / "packs" / "res",
                error);

            if (error)
            {
                error.clear();
            }

            const bool hasSys = std::filesystem::is_directory(
                current / "packs" / "sys",
                error);

            if (hasRes && hasSys)
            {
                return current;
            }

            const std::filesystem::path parent = current.parent_path();

            if (parent.empty() || parent == current)
            {
                break;
            }

            current = parent;
        }

        return {};
    }
}

namespace core::platform
{
    std::filesystem::path Paths::ExecutablePath()
    {
        std::vector<wchar_t> buffer(1024);

        for (;;)
        {
            const DWORD length = GetModuleFileNameW(
                nullptr,
                buffer.data(),
                static_cast<DWORD>(buffer.size()));

            if (length == 0)
            {
                return {};
            }

            if (length < buffer.size())
            {
                return std::filesystem::path(
                    std::wstring(buffer.data(), length));
            }

            buffer.resize(buffer.size() * 2);
        }
    }

    std::filesystem::path Paths::ExecutableDirectory()
    {
        const std::filesystem::path executablePath = ExecutablePath();

        if (executablePath.empty())
        {
            return {};
        }

        return executablePath.parent_path();
    }

    std::filesystem::path Paths::FindGameRoot()
    {
        const std::filesystem::path executableRoot =
            FindRootFrom(ExecutableDirectory());

        if (!executableRoot.empty())
        {
            return executableRoot;
        }

        std::error_code error;
        const std::filesystem::path workingDirectory =
            std::filesystem::current_path(error);

        if (error)
        {
            return {};
        }

        return FindRootFrom(workingDirectory);
    }
}