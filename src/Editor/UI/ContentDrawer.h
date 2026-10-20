#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace editor::ui {

    class ContentDrawer final {
    public:
        void draw();

    private:
        struct AssetItem {
            std::filesystem::path path;
            bool directory = false;
        };

        std::filesystem::path directory_;
        std::filesystem::path selected_;
        std::filesystem::path cachedDirectory_;
        std::vector<AssetItem> items_;
        std::vector<std::size_t> visibleItems_;
        std::string scanError_;
        std::string visibleFilter_;
        char filter_[128]{};
        bool searchAllFolders_ = false;
        bool cachedRecursive_ = false;
        bool cacheValid_ = false;
        bool visibleCacheValid_ = false;
    };

}
