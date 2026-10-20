#include "Editor/UI/ContentDrawer.h"

#include "Editor/Core/Paths.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace editor::ui {

    namespace {

        struct AssetItem {
            std::filesystem::path path;
            bool directory = false;
        };

        [[nodiscard]] std::string lowercase(std::string value) {
            for (char& character : value) {
                character = static_cast<char>(std::tolower(
                    static_cast<unsigned char>(character)
                ));
            }

            return value;
        }

    }

    void ContentDrawer::draw() {
        if (!ImGui::Begin("Content Drawer")) {
            ImGui::End();
            return;
        }

        const auto root = editor::paths::packs_directory();
        const auto current = root / directory_;

        if (ImGui::Button("Packs")) {
            directory_.clear();
            selected_.clear();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(directory_.empty());

        if (ImGui::Button("Up")) {
            directory_ = directory_.parent_path();
            selected_.clear();
        }

        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("/%s", directory_.generic_string().c_str());

        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputTextWithHint(
            "##content_filter",
            "Filter current folder",
            filter_,
            sizeof(filter_)
        );

        std::error_code error;
        std::vector<AssetItem> items;

        for (std::filesystem::directory_iterator it(current, error);
             !error && it != std::filesystem::directory_iterator();
             it.increment(error)) {
            const bool isDirectory = it->is_directory(error);

            if (error) {
                break;
            }

            if (!isDirectory && !it->is_regular_file(error)) {
                continue;
            }

            const auto relative = it->path().lexically_relative(root);

            if (!relative.empty()) {
                items.push_back({relative, isDirectory});
            }
        }

        std::sort(
            items.begin(),
            items.end(),
            [](const AssetItem& left, const AssetItem& right) {
                if (left.directory != right.directory) {
                    return left.directory;
                }

                return left.path.filename() < right.path.filename();
            }
        );

        if (error) {
            ImGui::TextWrapped("Cannot open folder: %s", error.message().c_str());
        }

        const std::string filter = lowercase(filter_);

        ImGui::BeginChild("##content_items", ImVec2(0.0f, -38.0f));

        for (const auto& item : items) {
            const std::string name = item.path.filename().string();

            if (!filter.empty() &&
                lowercase(name).find(filter) == std::string::npos) {
                continue;
            }

            const std::string label =
                (item.directory ? "[Folder] " : "[Asset] ") + name;

            if (ImGui::Selectable(
                label.c_str(),
                selected_ == item.path
            )) {
                if (item.directory) {
                    directory_ = item.path;
                    selected_.clear();
                } else {
                    selected_ = item.path;
                }
            }
        }

        ImGui::EndChild();

        if (!selected_.empty()) {
            ImGui::TextDisabled(
                "Selected: %s",
                selected_.generic_string().c_str()
            );
        }

        ImGui::End();
    }

}
