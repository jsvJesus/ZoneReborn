#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace engine::assets {

    struct ResourceFile {
        std::filesystem::path relative_path;
        std::uintmax_t size = 0;
    };

    struct FileTypeInfo {
        std::size_t count = 0;
        std::uintmax_t bytes = 0;
        std::vector<std::filesystem::path> samples;
    };

    class ResourceIndex {
    public:
        bool scan(const std::filesystem::path& root, std::string& error);

        const std::vector<ResourceFile>& files() const noexcept;
        const std::map<std::string, FileTypeInfo>& types() const noexcept;
        std::uintmax_t total_size() const noexcept;

    private:
        std::vector<ResourceFile> files_;
        std::map<std::string, FileTypeInfo> types_;
        std::uintmax_t total_size_ = 0;
    };

} // namespace engine::assets
