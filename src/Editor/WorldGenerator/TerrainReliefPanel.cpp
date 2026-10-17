#include "Editor/WorldGenerator/TerrainReliefPanel.h"

#include "WorldEditor/Terrain/TerrainReliefStorage.h"

#include "imgui.h"

#include <filesystem>

namespace editor::world_generator {

    namespace {

        using namespace world_editor::terrain;

        [[nodiscard]] std::filesystem::path relief_path(
            const std::filesystem::path& directory
        ) {
            auto path = directory;
            path += L".zrelief";
            return path;
        }

        [[nodiscard]] const char* preset_name(
            const ReliefPreset preset
        ) noexcept {
            switch (preset) {
            case ReliefPreset::Original:
                return "Original";
            case ReliefPreset::RollingHills:
                return "Rolling Hills";
            case ReliefPreset::Highlands:
                return "Highlands";
            case ReliefPreset::Mountains:
                return "Mountains";
            case ReliefPreset::Mixed:
                return "Mixed";
            }

            return "Unknown";
        }

    }

    void TerrainReliefPanel::draw() {
        if (!ImGui::CollapsingHeader(
            "02  Terrain Relief",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            return;
        }

        constexpr const char* presets[] = {
            "Original / Legacy",
            "Rolling Hills",
            "Highlands",
            "Mountains",
            "Mixed"
        };

        int selected = static_cast<int>(pending_.preset);

        ImGui::SetNextItemWidth(-1.0f);

        if (ImGui::Combo(
            "Relief Preset",
            &selected,
            presets,
            IM_ARRAYSIZE(presets)
        )) {
            pending_ = relief_preset(
                static_cast<ReliefPreset>(selected)
            );
        }

        if (pending_.preset != ReliefPreset::Original) {
            ImGui::SliderFloat(
                "Hill Height",
                &pending_.hillHeightMeters,
                0.0f,
                300.0f,
                "%.0f m"
            );

            ImGui::SliderFloat(
                "Mountain Height",
                &pending_.mountainHeightMeters,
                0.0f,
                1200.0f,
                "%.0f m"
            );

            ImGui::SliderFloat(
                "Mountain Coverage",
                &pending_.mountainCoverage,
                0.0f,
                1.0f,
                "%.2f"
            );

            ImGui::SliderFloat(
                "Ridge Sharpness",
                &pending_.ridgeSharpness,
                0.5f,
                5.0f,
                "%.2f"
            );
        }

        ImGui::Spacing();

        ImGui::TextDisabled(
            "Active Terrain: %s",
            preset_name(active_.preset)
        );

        ImGui::TextDisabled(
            "Changes apply after Generate Terrain."
        );
    }

    world_editor::terrain::Heightfield
    TerrainReliefPanel::generate(
        const world_editor::terrain::Settings& settings
    ) {
        auto generated = world_editor::terrain::generate_with_relief(
            settings,
            pending_
        );

        active_ = pending_;

        return generated;
    }

    void TerrainReliefPanel::save_for_world(
        const world_editor::terrain::Heightfield& terrain,
        const std::filesystem::path& directory
    ) const {
        const auto path = relief_path(directory);

        if (active_.preset ==
            world_editor::terrain::ReliefPreset::Original) {
            std::filesystem::remove(path);
            return;
        }

        world_editor::terrain::save_relief_profile(
            active_,
            terrain,
            path
        );
    }

    void TerrainReliefPanel::restore_from_world(
        const world_editor::terrain::Heightfield& terrain,
        const std::filesystem::path& directory
    ) {
        const auto path = relief_path(directory);

        world_editor::terrain::ReliefProfile restored =
            world_editor::terrain::relief_preset(
                world_editor::terrain::ReliefPreset::Original
            );

        if (std::filesystem::exists(path)) {
            restored = world_editor::terrain::load_relief_profile(
                terrain,
                path
            );
        }

        pending_ = restored;
        active_ = restored;
    }

    bool TerrainReliefPanel::has_saved_profile(
        const std::filesystem::path& directory
    ) {
        return std::filesystem::exists(
            relief_path(directory)
        );
    }

    void TerrainReliefPanel::reset_to_original() noexcept {
        active_ = world_editor::terrain::relief_preset(
            world_editor::terrain::ReliefPreset::Original
        );

        pending_ = active_;
    }
}
