#include "Character/CharacterService.h"

#include "Core/Log.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>

namespace
{
    constexpr char FileHeader[] =
        "CHARACTER_V1";

    bool ReadCodePoint(
        const std::string_view text,
        std::size_t& offset,
        std::uint32_t& codePoint)
    {
        if (offset >=
            text.size())
        {
            return false;
        }

        const auto first =
            static_cast<unsigned char>(
                text[offset++]);

        if (first <=
            0x7F)
        {
            codePoint =
                first;

            return true;
        }

        std::size_t continuationCount =
            0;

        if (first >= 0xC2 &&
            first <= 0xDF)
        {
            codePoint =
                first &
                0x1F;

            continuationCount =
                1;
        }
        else if (
            first >= 0xE0 &&
            first <= 0xEF)
        {
            codePoint =
                first &
                0x0F;

            continuationCount =
                2;
        }
        else if (
            first >= 0xF0 &&
            first <= 0xF4)
        {
            codePoint =
                first &
                0x07;

            continuationCount =
                3;
        }
        else
        {
            return false;
        }

        if (offset +
                continuationCount >
            text.size())
        {
            return false;
        }

        for (std::size_t index = 0;
             index < continuationCount;
             ++index)
        {
            const auto value =
                static_cast<unsigned char>(
                    text[offset++]);

            if ((value &
                 0xC0) !=
                0x80)
            {
                return false;
            }

            codePoint =
                (codePoint << 6u) |
                (
                    value &
                    0x3Fu
                );
        }

        if (continuationCount == 1 &&
            codePoint < 0x80)
        {
            return false;
        }

        if (continuationCount == 2 &&
            codePoint < 0x800)
        {
            return false;
        }

        if (continuationCount == 3 &&
            codePoint < 0x10000)
        {
            return false;
        }

        if (codePoint >
            0x10FFFF)
        {
            return false;
        }

        if (codePoint >= 0xD800 &&
            codePoint <= 0xDFFF)
        {
            return false;
        }

        return true;
    }

    bool IsAllowedNameCodePoint(
        const std::uint32_t value)
    {
        if (value >= 'A' &&
            value <= 'Z')
        {
            return true;
        }

        if (value >= 'a' &&
            value <= 'z')
        {
            return true;
        }

        if (value >= '0' &&
            value <= '9')
        {
            return true;
        }

        if (value ==
            '_')
        {
            return true;
        }

        return
            value >= 0x0400 &&
            value <= 0x052F;
    }
}

namespace client::character
{
    bool Service::Initialize(
        const std::filesystem::path& gameRoot)
    {
        if (gameRoot.empty())
        {
            return false;
        }

        storageDirectory_ =
            gameRoot /
            "user" /
            "characters";

        std::error_code error;

        std::filesystem::create_directories(
            storageDirectory_,
            error);

        return
            !error;
    }

    bool Service::Load(
        const std::string_view accountLogin,
        std::optional<Profile>& profile,
        std::string& error) const
    {
        profile.reset();
        error.clear();

        if (storageDirectory_.empty())
        {
            error =
                "Character storage is not initialized.";

            return false;
        }

        if (accountLogin.empty())
        {
            error =
                "Account login is empty.";

            return false;
        }

        const std::filesystem::path path =
            AccountPath(
                accountLogin);

        const std::filesystem::path legacyPath =
            LegacyAccountPath(
                accountLogin);

        core::Log::Info(
            std::string(
                "Character storage directory: ") +
            storageDirectory_.string());

        core::Log::Info(
            std::string(
                "Character profile path: ") +
            path.string());

        std::error_code filesystemError;

        bool exists =
            std::filesystem::exists(
                path,
                filesystemError);

        if (filesystemError)
        {
            error =
                "Unable to access character storage.";

            return false;
        }

        if (!exists)
        {
            const bool legacyExists =
                std::filesystem::exists(
                    legacyPath,
                    filesystemError);

            if (filesystemError)
            {
                error =
                    "Unable to access legacy character storage.";

                return false;
            }

            if (legacyExists)
            {
                core::Log::Info(
                    std::string(
                        "Legacy character profile found: ") +
                    legacyPath.string());

                Profile legacyProfile;

                if (!ReadProfile(
                        legacyPath,
                        legacyProfile,
                        error))
                {
                    return false;
                }

                if (!WriteProfile(
                        path,
                        legacyProfile,
                        error))
                {
                    return false;
                }

                std::error_code removeError;

                std::filesystem::remove(
                    legacyPath,
                    removeError);

                profile =
                    std::move(
                        legacyProfile);

                core::Log::Info(
                    std::string(
                        "Legacy character profile migrated to: ") +
                    path.string());

                return true;
            }

            core::Log::Info(
                "Character profile does not exist.");

            return true;
        }

        Profile loaded;

        if (!ReadProfile(
                path,
                loaded,
                error))
        {
            return false;
        }

        core::Log::Info(
            std::string(
                "Character profile loaded from: ") +
            path.string());

        profile =
            std::move(
                loaded);

        return true;
    }

    bool Service::Create(
        const std::string_view accountLogin,
        const std::string_view name,
        const Catalog& catalog,
        const std::vector<
            std::pair<
                std::string,
                std::int32_t>>& appearance,
        Profile& profile,
        std::string& error) const
    {
        error.clear();

        std::optional<Profile>
            existing;

        if (!Load(
                accountLogin,
                existing,
                error))
        {
            return false;
        }

        if (existing.has_value())
        {
            error =
                "Character already exists.";

            return false;
        }

        if (!ValidateName(
                name,
                error))
        {
            return false;
        }

        Profile created;

        created.id =
            GenerateId();

        created.name =
            std::string(
                name);

        for (const CreatorGroup& group :
             catalog.CreatorGroups())
        {
            if (group.options.empty())
            {
                continue;
            }

            AppearancePart part;

            part.group =
                group.name;

            const auto selected =
                std::find_if(
                    appearance.begin(),
                    appearance.end(),
                    [&group](
                        const auto& value)
                    {
                        return
                            value.first ==
                                group.name;
                    });

            if (selected ==
                appearance.end())
            {
                part.itemType =
                    group.options.front().
                        itemType;
            }
            else
            {
                const auto allowed =
                    std::find_if(
                        group.options.begin(),
                        group.options.end(),
                        [selected](
                            const CreatorOption& option)
                        {
                            return
                                option.itemType ==
                                    selected->second;
                        });

                if (allowed ==
                    group.options.end())
                {
                    error =
                        "Invalid character creator item for group " +
                        group.name +
                        ".";

                    return false;
                }

                part.itemType =
                    selected->second;
            }

            created.appearance.
                push_back(
                    std::move(
                        part));
        }

        const std::filesystem::path path =
            AccountPath(
                accountLogin);

        if (!WriteProfile(
                path,
                created,
                error))
        {
            return false;
        }

        core::Log::Info(
            std::string(
                "Character profile saved: ") +
            path.string());

        profile =
            std::move(
                created);

        return true;
    }
    
    bool Service::Delete(
        const std::string_view accountLogin,
        std::string& error) const
    {
        error.clear();

        if (storageDirectory_.empty())
        {
            error =
                "Character storage is not initialized.";

            return false;
        }

        if (accountLogin.empty())
        {
            error =
                "Account login is empty.";

            return false;
        }

        const std::filesystem::path path =
            AccountPath(
                accountLogin);

        const std::filesystem::path legacyPath =
            LegacyAccountPath(
                accountLogin);

        std::error_code filesystemError;

        bool removed =
            false;

        if (std::filesystem::exists(
                path,
                filesystemError))
        {
            if (filesystemError)
            {
                error =
                    "Unable to access character storage.";

                return false;
            }

            removed =
                std::filesystem::remove(
                    path,
                    filesystemError);

            if (filesystemError)
            {
                error =
                    "Unable to delete character.";

                return false;
            }
        }

        filesystemError.clear();

        if (std::filesystem::exists(
                legacyPath,
                filesystemError))
        {
            if (filesystemError)
            {
                error =
                    "Unable to access legacy character storage.";

                return false;
            }

            const bool legacyRemoved =
                std::filesystem::remove(
                    legacyPath,
                    filesystemError);

            if (filesystemError)
            {
                error =
                    "Unable to delete legacy character.";

                return false;
            }

            removed =
                removed ||
                legacyRemoved;
        }

        if (!removed)
        {
            error =
                "Character does not exist.";

            return false;
        }

        return true;
    }

    std::filesystem::path Service::AccountPath(
    const std::string_view accountLogin) const
    {
        return
            storageDirectory_ /
            (
                AccountKey(
                    accountLogin) +
                ".dat"
            );
    }

    std::filesystem::path Service::LegacyAccountPath(const std::string_view accountLogin) const
    {
        return
            storageDirectory_ /
            (
                LegacyAccountKey(
                    accountLogin) +
                ".dat"
            );
    }

    std::string Service::AccountKey(const std::string_view accountLogin)
    {
        std::string result;

        result.reserve(
            accountLogin.size());

        for (const unsigned char character :
             accountLogin)
        {
            if (
                (
                    character >= 'A' &&
                    character <= 'Z'
                ) ||
                (
                    character >= 'a' &&
                    character <= 'z'
                ) ||
                (
                    character >= '0' &&
                    character <= '9'
                ) ||
                character == '_' ||
                character == '-')
            {
                result.push_back(
                    static_cast<char>(
                        character));
            }
            else
            {
                result.push_back(
                    '_');
            }
        }

        if (result.empty())
        {
            result =
                "account";
        }

        return result;
    }

    std::string Service::LegacyAccountKey(const std::string_view accountLogin)
    {
        static constexpr char Hex[] =
            "0123456789abcdef";

        std::string result;

        result.reserve(
            accountLogin.size() *
            2);

        for (const unsigned char value :
             accountLogin)
        {
            result.push_back(
                Hex[
                    value >>
                    4u]);

            result.push_back(
                Hex[
                    value &
                    0x0Fu]);
        }

        return result;
    }

    std::string Service::GenerateId()
    {
        std::random_device device;

        std::mt19937_64 generator(
            (
                static_cast<std::uint64_t>(
                    device()) <<
                32u
            ) ^
            static_cast<std::uint64_t>(
                device()));

        const std::uint64_t high =
            generator();

        const std::uint64_t low =
            generator();

        std::ostringstream stream;

        stream <<
            std::hex <<
            std::setfill('0') <<
            std::setw(16) <<
            high <<
            std::setw(16) <<
            low;

        return
            stream.str();
    }

    bool Service::ValidateName(
        const std::string_view name,
        std::string& error)
    {
        error.clear();

        if (name.empty())
        {
            error =
                "Character name is empty.";

            return false;
        }

        std::size_t offset =
            0;

        std::size_t characterCount =
            0;

        std::size_t underscoreCount =
            0;

        bool lastWasUnderscore =
            false;

        while (offset <
               name.size())
        {
            std::uint32_t codePoint =
                0;

            if (!ReadCodePoint(
                    name,
                    offset,
                    codePoint))
            {
                error =
                    "Character name contains invalid UTF-8.";

                return false;
            }

            ++characterCount;

            if (!IsAllowedNameCodePoint(
                    codePoint))
            {
                error =
                    "Character name contains unsupported characters.";

                return false;
            }

            if (codePoint ==
                '_')
            {
                ++underscoreCount;

                if (characterCount ==
                    1)
                {
                    error =
                        "Character name cannot start with underscore.";

                    return false;
                }

                if (underscoreCount >
                    1)
                {
                    error =
                        "Character name may contain only one underscore.";

                    return false;
                }

                lastWasUnderscore =
                    true;
            }
            else
            {
                lastWasUnderscore =
                    false;
            }
        }

        if (characterCount <
            3)
        {
            error =
                "Character name must contain at least 3 characters.";

            return false;
        }

        if (characterCount >
            32)
        {
            error =
                "Character name may contain no more than 32 characters.";

            return false;
        }

        if (lastWasUnderscore)
        {
            error =
                "Character name cannot end with underscore.";

            return false;
        }

        return true;
    }

    bool Service::ReadProfile(
        const std::filesystem::path& path,
        Profile& profile,
        std::string& error)
    {
        error.clear();

        std::ifstream stream(
            path,
            std::ios::binary);

        if (!stream)
        {
            error =
                "Unable to open character file.";

            return false;
        }

        std::string header;

        std::getline(
            stream,
            header);

        if (header !=
            FileHeader)
        {
            error =
                "Unsupported character file format.";

            return false;
        }

        Profile loaded;

        std::size_t appearanceCount =
            0;

        if (!(stream >>
              std::quoted(
                  loaded.id)))
        {
            error =
                "Unable to read character id.";

            return false;
        }

        if (!(stream >>
              std::quoted(
                  loaded.name)))
        {
            error =
                "Unable to read character name.";

            return false;
        }

        if (!(stream >>
              appearanceCount))
        {
            error =
                "Unable to read character appearance.";

            return false;
        }

        if (appearanceCount >
            256)
        {
            error =
                "Character appearance is invalid.";

            return false;
        }

        loaded.appearance.reserve(
            appearanceCount);

        for (std::size_t index = 0;
             index < appearanceCount;
             ++index)
        {
            AppearancePart part;

            if (!(stream >>
                  std::quoted(
                      part.group) >>
                  part.itemType))
            {
                error =
                    "Unable to read character appearance part.";

                return false;
            }

            if (part.group.empty() ||
                part.itemType <= 0)
            {
                error =
                    "Character appearance part is invalid.";

                return false;
            }

            loaded.appearance.
                push_back(
                    std::move(
                        part));
        }

        if (loaded.id.empty() ||
            loaded.name.empty())
        {
            error =
                "Character profile is incomplete.";

            return false;
        }

        profile =
            std::move(
                loaded);

        return true;
    }

    bool Service::WriteProfile(
        const std::filesystem::path& path,
        const Profile& profile,
        std::string& error)
    {
        error.clear();

        std::ofstream stream(
            path,
            std::ios::binary |
            std::ios::trunc);

        if (!stream)
        {
            error =
                "Unable to create character file.";

            return false;
        }

        stream <<
            FileHeader <<
            '\n';

        stream <<
            std::quoted(
                profile.id) <<
            '\n';

        stream <<
            std::quoted(
                profile.name) <<
            '\n';

        stream <<
            profile.appearance.size() <<
            '\n';

        for (const AppearancePart& part :
             profile.appearance)
        {
            stream <<
                std::quoted(
                    part.group) <<
                ' ' <<
                part.itemType <<
                '\n';
        }

        stream.flush();

        if (!stream)
        {
            error =
                "Unable to save character.";

            return false;
        }

        return true;
    }
}
