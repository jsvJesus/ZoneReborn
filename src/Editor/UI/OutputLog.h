#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace editor::ui {

    class OutputLog final {
    public:
        void add(std::string_view message);
        void draw();

    private:
        std::vector<std::string> entries_;
    };

}
