#include "Editor/WorldGenerator/WorldStoragePanel.h"

#include "Editor/WorldGenerator/MapLayoutPanel.h"
#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/Core/Paths.h"

#include "WorldEditor/Terrain/TerrainWorldStorage.h"

#include "imgui.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace editor::world_generator {

    namespace {

        [[nodiscard]] bool valid_name(
            const std::string_view name
        ) noexcept {
            if (name.empty() || name.size() > 64) {
                return false;
            }

            for (const char ch : name) {
                const bool letter =
                    (ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z');

                const bool digit =
                    ch >= '0' && ch <= '9';

                if (!letter && !digit && ch != '_' && ch != '-') {
                    return false;
                }
            }

            return true;
        }

    }

    std::filesystem::path
        WorldStoragePanel::world_directory() const {
        if (!valid_name(worldName_)) {
            throw std::invalid_argument(
                "World name must contain only "
                "Latin letters, digits, _ or -"
            );
        }

        return editor::paths::worlds_directory() / worldName_;
    }

    void WorldStoragePanel::draw(
        world_editor::terrain::Heightfield& terrain,
        world_editor::terrain::Settings& settings,
        world_editor::terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport,
        MapLayoutPanel& mapPanel
    ) {
        ImGui::Begin("World Storage");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "WORLD STORAGE"
        );

        ImGui::TextDisabled(
            "Terrain / Chunks / Streaming"
        );

        ImGui::Separator();

        ImGui::SetNextItemWidth(260.0f);

        ImGui::InputText(
            "World Name",
            worldName_,
            sizeof(worldName_)
        );

        ImGui::Spacing();

        ImGui::BeginDisabled(viewport.is_streaming());

        if (ImGui::Button("Save World")) {
            try {
                const auto directory = world_directory();

                world_editor::terrain::save_world(
                    terrain,
                    activeSettings,
                    directory
                );

                mapPanel.save_hydrology_for_world(
                    terrain,
                    activeSettings,
                    directory
                );

                status_ = "World saved";
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
                const auto directory = world_directory();

                auto loaded =
                    world_editor::terrain::load_world(directory);

                viewport.set_terrain(loaded.terrain);

                mapPanel.clear(viewport);

                terrain = std::move(loaded.terrain);
                activeSettings = loaded.settings;
                settings = loaded.settings;

                const bool waterRestored = mapPanel.restore_hydrology(
                    terrain,
                    activeSettings,
                    viewport,
                    directory
                );

                status_ = waterRestored
                    ? "World and Hydrology loaded"
                    : "World loaded (no Hydrology file)";
                
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Stream World")) {
            try {
                const auto directory = world_directory();

                viewport.open_streamed_world(directory);

                mapPanel.clear(viewport);

                settings = viewport.streaming_settings();
                activeSettings = settings;
                terrain = {};

                std::filesystem::path waterPath = directory;
                waterPath += L".zhydro";

                bool waterRestored = false;

                if (std::filesystem::exists(waterPath)) {
                    // Temporary CPU reconstruction for Hydrology v1.
                    // The GPU terrain continues to use chunk streaming.
                    auto source =
                        world_editor::terrain::load_world(directory);

                    waterRestored = mapPanel.restore_hydrology(
                        source.terrain,
                        source.settings,
                        viewport,
                        directory
                    );
                }

                status_ = waterRestored
                    ? "World Streaming and Hydrology enabled"
                    : "World Streaming enabled";
                
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        if (viewport.is_streaming()) {
            ImGui::TextDisabled(
                "Streaming mode / Terrain saving disabled"
            );
        }

        if (!status_.empty()) {
            ImGui::TextColored(
                hasError_
                    ? ImVec4(1.0f, 0.48f, 0.42f, 1.0f)
                    : ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
                "%s",
                status_.c_str()
            );
        }

        ImGui::End();
    }

}
