#pragma once

#include "Character/CharacterCatalog.h"
#include "Character/CharacterProfile.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace client::character
{
    class Service final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            const std::filesystem::path& gameRoot);

        [[nodiscard]]
        bool Load(
            std::string_view accountLogin,
            std::optional<Profile>& profile,
            std::string& error) const;

        [[nodiscard]]
        bool Create(
            std::string_view accountLogin,
            std::string_view name,
            const Catalog& catalog,
            const std::vector<
                std::pair<
                    std::string,
                    std::int32_t>>& appearance,
            Profile& profile,
            std::string& error) const;

        [[nodiscard]]
        bool Delete(
            std::string_view accountLogin,
            std::string& error) const;

    private:
        [[nodiscard]]
        std::filesystem::path AccountPath(
            std::string_view accountLogin) const;

        [[nodiscard]]
        std::filesystem::path LegacyAccountPath(
            std::string_view accountLogin) const;

        [[nodiscard]]
        static std::string AccountKey(
            std::string_view accountLogin);

        [[nodiscard]]
        static std::string LegacyAccountKey(
            std::string_view accountLogin);

        [[nodiscard]]
        static std::string GenerateId();

        [[nodiscard]]
        static bool ValidateName(
            std::string_view name,
            std::string& error);

        [[nodiscard]]
        static bool ReadProfile(
            const std::filesystem::path& path,
            Profile& profile,
            std::string& error);

        [[nodiscard]]
        static bool WriteProfile(
            const std::filesystem::path& path,
            const Profile& profile,
            std::string& error);

        std::filesystem::path
            storageDirectory_;
    };
}
