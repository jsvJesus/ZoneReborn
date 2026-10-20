#pragma once

#include <filesystem>
#include <string>

namespace editor::ui {

    class ContentDrawer final {
    public:
        void draw();

    private:
        std::filesystem::path directory_;
        std::filesystem::path selected_;
        char filter_[128]{};
    };

}
