#include "Core/Resources/ResourcePath.h"

#include <string>

namespace
{
    char ToLowerAscii(
        const char value) noexcept
    {
        if (value >= 'A' &&
            value <= 'Z')
        {
            return static_cast<char>(
                value - 'A' + 'a');
        }

        return value;
    }
}

namespace core::resources
{
    bool ResourcePath::IsValid(
        const std::string_view path) noexcept
    {
        if (path.empty())
        {
            return false;
        }

        if (path.find("..") !=
            std::string_view::npos)
        {
            return false;
        }

        if (path.front() == '/' ||
            path.front() == '\\')
        {
            return false;
        }

        if (path.size() >= 2 &&
            path[1] == ':')
        {
            return false;
        }

        return true;
    }

    std::string ResourcePath::Normalize(
        const std::string_view path)
    {
        if (!IsValid(path))
        {
            return {};
        }

        std::string result;

        result.reserve(
            path.size());

        bool previousWasSlash = false;

        for (const char sourceCharacter : path)
        {
            char character =
                sourceCharacter;

            if (character == '\\')
            {
                character = '/';
            }

            if (character == '/')
            {
                if (previousWasSlash)
                {
                    continue;
                }

                previousWasSlash = true;
                result.push_back('/');
                continue;
            }

            previousWasSlash = false;

            result.push_back(
                ToLowerAscii(character));
        }

        while (!result.empty() &&
               result.back() == '/')
        {
            result.pop_back();
        }

        return result;
    }

    std::string ResourcePath::ToResPath(
        const std::string_view path)
    {
        std::string normalized =
            Normalize(path);

        if (normalized.empty())
        {
            return {};
        }

        if (normalized.starts_with("res/"))
        {
            return normalized;
        }

        std::string result;

        result.reserve(
            4 + normalized.size());

        result =
            "res/";

        result +=
            normalized;

        return result;
    }
}