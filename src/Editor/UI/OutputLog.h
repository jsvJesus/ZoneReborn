#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace editor::ui {

    enum class LogLevel {
        Info,
        Warning,
        Error
    };

    class OutputLog final {
    public:
        void add(std::string_view message, LogLevel level = LogLevel::Info);
        void draw();

    private:
        struct Entry {
            std::string time;
            std::string message;
            LogLevel level = LogLevel::Info;
        };

        std::vector<Entry> entries_;
        char filter_[128]{};
        bool showInfo_ = true;
        bool showWarnings_ = true;
        bool showErrors_ = true;
        bool autoScroll_ = true;
        bool scrollToBottom_ = false;
    };

}
