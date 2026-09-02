#pragma once

#include <filesystem>

namespace core::platform
{
    class Paths final
    {
    public:
        [[nodiscard]]
        static std::filesystem::path ExecutablePath();

        [[nodiscard]]
        static std::filesystem::path ExecutableDirectory();

        [[nodiscard]]
        static std::filesystem::path FindGameRoot();
    };
}