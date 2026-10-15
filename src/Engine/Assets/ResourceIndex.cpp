#include "Engine/Assets/ResourceIndex.h"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace engine::assets {

bool ResourceIndex::scan(const std::filesystem::path& root, std::string& error) {
    files_.clear();
    types_.clear();
    total_size_ = 0;
    error.clear();

    std::error_code ec;
    const auto directory = std::filesystem::absolute(root, ec);

    if (ec) {
        error = "Cannot resolve packs directory: " + ec.message();
        return false;
    }

    if (!std::filesystem::is_directory(directory, ec)) {
        error = ec ? ec.message() : "Packs directory does not exist: " + directory.string();
        return false;
    }

    std::filesystem::recursive_directory_iterator it(directory, ec), end;

    if (ec) {
        error = "Cannot open packs directory: " + ec.message();
        return false;
    }

    while (it != end) {
        const bool regular = it->is_regular_file(ec);

        if (ec) {
            error = "Cannot inspect resource: " + ec.message();
            return false;
        }

        if (regular) {
            const auto size = it->file_size(ec);

            if (ec) {
                error = "Cannot read resource size: " + ec.message();
                return false;
            }

            auto relative_path = it->path().lexically_relative(directory);
            auto extension = relative_path.extension().string();

            if (extension.empty()) {
                extension = "(none)";
            } else {
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
            }

            files_.push_back({relative_path, size});

            auto& type = types_[extension];
            ++type.count;
            type.bytes += size;

            if (type.samples.size() < 3) {
                type.samples.push_back(std::move(relative_path));
            }

            total_size_ += size;
        }

        it.increment(ec);

        if (ec) {
            error = "Cannot continue resource scan: " + ec.message();
            return false;
        }
    }

    std::sort(files_.begin(), files_.end(),
        [](const ResourceFile& a, const ResourceFile& b) {
            return a.relative_path.generic_string() < b.relative_path.generic_string();
        });

    return true;
}

const std::vector<ResourceFile>& ResourceIndex::files() const noexcept {
    return files_;
}

const std::map<std::string, FileTypeInfo>& ResourceIndex::types() const noexcept {
    return types_;
}

std::uintmax_t ResourceIndex::total_size() const noexcept {
    return total_size_;
}

} // namespace engine::assets
