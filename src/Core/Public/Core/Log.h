#pragma once

#include <string_view>

namespace core
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    class Log final
    {
    public:
        static void Write(LogLevel level, std::string_view message);

        static void Info(std::string_view message);
        static void Warning(std::string_view message);
        static void Error(std::string_view message);

    private:
        static const char* LevelName(LogLevel level) noexcept;
    };
}