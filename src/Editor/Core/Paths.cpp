
#include "Editor/Core/Paths.h"

#include <Windows.h>

#include <stdexcept>
#include <string>

namespace editor::paths {

    const std::filesystem::path& executable_directory() {
        static const std::filesystem::path directory = [] {
            std::wstring executablePath(32768, L'\0');

            const DWORD length = GetModuleFileNameW(
                nullptr,
                executablePath.data(),
                static_cast<DWORD>(executablePath.size())
            );

            if (length == 0 || length >= executablePath.size()) {
                throw std::runtime_error(
                    "Failed to resolve executable directory"
                );
            }

            executablePath.resize(length);

            return std::filesystem::path(executablePath).parent_path();
        }();

        return directory;
    }

    std::filesystem::path worlds_directory() {
        return executable_directory() / "worlds";
    }

    std::filesystem::path generated_directory() {
        return executable_directory() / "generated";
    }

    std::filesystem::path imgui_settings_path() {
        return executable_directory() / "imgui_v3.ini";
    }

    std::filesystem::path packs_directory() {
        return executable_directory().parent_path() / "packs";
    }

}
