
#include "Editor/WorldGenerator/WorldStoragePanel.h"

#include "Editor/Viewport/TerrainViewport.h"
#include "WorldEditor/Terrain/TerrainWorldStorage.h"

#include "Editor/Core/Paths.h"

#include "imgui.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace editor::world_generator {

    namespace {

        bool valid_name(std::string_view name) noexcept {
            if (name.empty() || name.size() > 64) {
                return false;
            }

            for (const char ch : name) {
                const bool letter =
                    (ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z');

                const bool digit = ch >= '0' && ch <= '9';

                if (!letter && !digit && ch != '_' && ch != '-') {
                    return false;
                }
            }

            return true;
        }

        std::filesystem::path world_directory(const char* name) {
            if (!valid_name(name)) {
                throw std::invalid_argument(
                    "World name must contain only Latin letters, digits, _ or -"
                );
            }

            return editor::paths::worlds_directory() / name;
        }

    }

    void WorldStoragePanel::draw(
        world_editor::terrain::Heightfield& terrain,
        world_editor::terrain::Settings& settings,
        world_editor::terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport
    ) {
        ImGui::Separator();
        ImGui::TextUnformatted("World Storage");

        ImGui::InputText(
            "World Name",
            worldName_,
            sizeof(worldName_)
        );

        ImGui::BeginDisabled(viewport.is_streaming());

        if (ImGui::Button("Save World")) {
            try {
                const auto directory = world_directory(worldName_);

                world_editor::terrain::save_world(
                    terrain,
                    activeSettings,
                    directory
                );

                status_ = "World saved: " + std::string(worldName_);
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::Button("Load World")) {
            try {
                const auto directory = world_directory(worldName_);
                auto loaded = world_editor::terrain::load_world(directory);

                viewport.set_terrain(loaded.terrain);

                terrain = std::move(loaded.terrain);
                activeSettings = loaded.settings;
                settings = loaded.settings;

                status_ = "World loaded: " + std::string(worldName_);
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }
        
        if (ImGui::Button("Stream World")) {
            try {
                const auto directory = world_directory(worldName_);

                viewport.open_streamed_world(directory);

                settings = viewport.streaming_settings();
                activeSettings = settings;

                terrain = {};

                status_ = "Streaming world: " + std::string(worldName_);
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        if (viewport.is_streaming()) {
            ImGui::TextDisabled(
                "Streaming mode: Save World is disabled."
            );
        }

        if (!status_.empty()) {
            ImGui::TextWrapped(
                "%s: %s",
                hasError_ ? "Error" : "OK",
                status_.c_str()
            );
        }
    }

}
