#include "Core/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace
{
    std::mutex logMutex;
}

namespace core
{
    void Log::Write(const LogLevel level, const std::string_view message)
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);

        std::tm localTime{};
        localtime_s(&localTime, &time);

        std::scoped_lock lock(logMutex);

        std::cout
            << '['
            << std::put_time(&localTime, "%H:%M:%S")
            << "] ["
            << LevelName(level)
            << "] "
            << message
            << '\n';
    }

    void Log::Info(const std::string_view message)
    {
        Write(LogLevel::Info, message);
    }

    void Log::Warning(const std::string_view message)
    {
        Write(LogLevel::Warning, message);
    }

    void Log::Error(const std::string_view message)
    {
        Write(LogLevel::Error, message);
    }

    const char* Log::LevelName(const LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::Info:
            return "INFO";

        case LogLevel::Warning:
            return "WARNING";

        case LogLevel::Error:
            return "ERROR";
        }

        return "UNKNOWN";
    }
}