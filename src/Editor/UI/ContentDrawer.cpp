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

        const auto root = editor::paths::packs_directory() / "models";
        const auto current = root / directory_;

        if (ImGui::Button("Models")) {
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
        ImGui::SameLine();

        if (ImGui::SmallButton("Refresh")) {
            cacheValid_ = false;
        }

        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputTextWithHint(
            "##content_filter",
            "Search models",
            filter_,
            sizeof(filter_)
        );
        ImGui::SameLine();
        ImGui::Checkbox("All folders", &searchAllFolders_);

        const bool recursive = searchAllFolders_ && filter_[0] != '\0';
        const auto scanDirectory = recursive ? root : current;

        if (!cacheValid_ || cachedDirectory_ != scanDirectory ||
            cachedRecursive_ != recursive) {
            std::error_code error;
            items_.clear();
            scanError_.clear();

            if (recursive) {
                for (std::filesystem::recursive_directory_iterator it(root, error);
                     !error && it != std::filesystem::recursive_directory_iterator();
                     it.increment(error)) {
                    if (it->is_regular_file(error) &&
                        lowercase(it->path().extension().string()) == ".model") {
                        items_.push_back({it->path().lexically_relative(root), false});
                    }
                }
            } else {
                for (std::filesystem::directory_iterator it(current, error);
                     !error && it != std::filesystem::directory_iterator();
                     it.increment(error)) {
                    const bool isDirectory = it->is_directory(error);

                    if (error) {
                        break;
                    }

                    if (isDirectory ||
                        (it->is_regular_file(error) &&
                         lowercase(it->path().extension().string()) == ".model")) {
                        items_.push_back({it->path().lexically_relative(root), isDirectory});
                    }
                }
            }

            std::sort(
                items_.begin(),
                items_.end(),
                [](const AssetItem& left, const AssetItem& right) {
                    if (left.directory != right.directory) {
                        return left.directory;
                    }

                    return left.path < right.path;
                }
            );

            if (error) {
                scanError_ = error.message();
            }

            cachedDirectory_ = scanDirectory;
            cachedRecursive_ = recursive;
            cacheValid_ = true;
            visibleCacheValid_ = false;
        }

        if (!scanError_.empty()) {
            ImGui::TextWrapped("Cannot open folder: %s", scanError_.c_str());
        }

        const std::string filter = lowercase(filter_);

        if (!visibleCacheValid_ || visibleFilter_ != filter) {
            visibleItems_.clear();

            for (std::size_t index = 0; index < items_.size(); ++index) {
                const auto& item = items_[index];
                const std::string name = recursive
                    ? item.path.generic_string()
                    : item.path.filename().string();

                if (filter.empty() ||
                    lowercase(name).find(filter) != std::string::npos) {
                    visibleItems_.push_back(index);
                }
            }

            visibleFilter_ = filter;
            visibleCacheValid_ = true;
        }

        ImGui::TextDisabled("%zu models and folders", visibleItems_.size());
        ImGui::BeginChild("##content_items", ImVec2(0.0f, -48.0f));

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visibleItems_.size()));

        while (clipper.Step()) {
            for (int index = clipper.DisplayStart;
                 index < clipper.DisplayEnd;
                 ++index) {
                const auto& item = items_[visibleItems_[index]];
                const std::string name = recursive
                    ? item.path.generic_string()
                    : item.path.filename().string();

                const std::string label =
                    (item.directory ? "[Folder] " : "[Model] ") + name;

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
        }

        ImGui::EndChild();

        if (!selected_.empty()) {
            ImGui::TextDisabled("%s", selected_.generic_string().c_str());
            ImGui::SameLine();

            if (ImGui::SmallButton("Copy Path")) {
                const std::string path = (root / selected_).string();
                ImGui::SetClipboardText(path.c_str());
            }
        }

        ImGui::End();
    }

}
