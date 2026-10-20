#pragma once

#include <filesystem>

namespace editor::paths {

    [[nodiscard]] const std::filesystem::path& executable_directory();

    [[nodiscard]] std::filesystem::path worlds_directory();

    [[nodiscard]] std::filesystem::path generated_directory();

    [[nodiscard]] std::filesystem::path imgui_settings_path();

    [[nodiscard]] std::filesystem::path vegetation_settings_path();

    [[nodiscard]] std::filesystem::path packs_directory();

}
