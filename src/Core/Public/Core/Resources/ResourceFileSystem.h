#pragma once

#include "Core/Resources/ResourceType.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace core::resources
{
    struct ResourceEntry final
    {
        std::string logicalPath;
        std::filesystem::path physicalPath;

        ResourceType type = ResourceType::Unknown;

        std::uintmax_t size = 0;
    };

    class ResourceFileSystem final
    {
    public:
        ResourceFileSystem() = default;

        ResourceFileSystem(const ResourceFileSystem&) = delete;
        ResourceFileSystem& operator=(const ResourceFileSystem&) = delete;

        ResourceFileSystem(ResourceFileSystem&&) = delete;
        ResourceFileSystem& operator=(ResourceFileSystem&&) = delete;

        [[nodiscard]]
        bool Initialize(
            const std::filesystem::path& packsRoot);

        void Shutdown();

        [[nodiscard]]
        bool IsInitialized() const noexcept;

        [[nodiscard]]
        bool Exists(
            std::string_view logicalPath) const;

        [[nodiscard]]
        const ResourceEntry* Find(
            std::string_view logicalPath) const;

        [[nodiscard]]
        bool ReadBinary(
            std::string_view logicalPath,
            std::vector<std::byte>& output) const;

        [[nodiscard]]
        bool ReadText(
            std::string_view logicalPath,
            std::string& output) const;

        [[nodiscard]]
        std::size_t ResourceCount() const noexcept;

        [[nodiscard]]
        std::size_t ResourceCount(
            ResourceType type) const noexcept;

        [[nodiscard]]
        std::vector<const ResourceEntry*> FindByType(
            ResourceType type) const;

        [[nodiscard]]
        const std::filesystem::path& PacksRoot() const noexcept;

    private:
        using ResourceIndex =
            std::unordered_map<std::string, ResourceEntry>;

        bool IndexMount(
            std::string_view mountName,
            const std::filesystem::path& directory);

        [[nodiscard]]
        static std::string NormalizeLogicalPath(
            std::string_view logicalPath);

        std::filesystem::path packsRoot_;
        ResourceIndex index_;

        bool initialized_ = false;
    };
}