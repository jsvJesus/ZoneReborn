#include "Editor/WorldGenerator/BiomeMaterialsPanel.h"

#include "Editor/Viewport/TerrainViewport.h"

#include "WorldEditor/Biome/BiomeStorage.h"

#include "imgui.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace editor::world_generator {

    namespace {

        namespace biome = world_editor::biome;

    }

    void BiomeMaterialsPanel::draw(
        const world_editor::terrain::Heightfield& terrain,
        viewport::TerrainViewport& viewport
    ) {
        ImGui::Begin("Biome Materials");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "BIOME GENERATOR"
        );

        ImGui::TextDisabled(
            "Grass / Soil / Sand / Rock / Marsh"
        );

        ImGui::Separator();

        ImGui::InputScalar(
            "Biome / Vegetation Seed",
            ImGuiDataType_U32,
            &pending_.seed
        );

        ImGui::SliderFloat(
            "Moisture",
            &pending_.moisture,
            0.0f,
            1.0f,
            "%.2f"
        );

        ImGui::SliderFloat(
            "Forest Coverage",
            &pending_.forestCoverage,
            0.0f,
            1.0f,
            "%.2f"
        );

        ImGui::SliderFloat(
            "Rock Slope",
            &pending_.rockSlopeDegrees,
            5.0f,
            60.0f,
            "%.1f deg"
        );

        ImGui::SliderFloat(
            "Material Tile Size",
            &pending_.textureScaleMeters,
            2.0f,
            32.0f,
            "%.1f m"
        );

        ImGui::Spacing();

        if (viewport.is_streaming()) {
            ImGui::TextWrapped(
                "Streaming mode: use saved Biome data. "
                "Switch to Load World to regenerate."
            );
        }

        ImGui::BeginDisabled(
            !terrain.valid() ||
            viewport.is_streaming()
        );

        if (ImGui::Button(
            "Generate Biomes + Apply Materials",
            ImVec2(-1.0f, 42.0f)
        )) {
            try {
                auto generated = biome::generate_biomes(
                    terrain,
                    viewport.hydrology(),
                    pending_
                );

                viewport.set_biomes(generated);

                sourceFingerprint_ =
                    biome::biome_source_fingerprint(
                        terrain,
                        viewport.hydrology()
                    );

                active_ = std::move(generated);

                status_ = "Biome materials applied";
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        ImGui::EndDisabled();

        if (active_) {
            ImGui::Separator();

            ImGui::Text(
                "Biome grid: %u x %u",
                active_->samplesX,
                active_->samplesZ
            );

            ImGui::Text(
                "Active Seed: %u",
                active_->settings.seed
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

    void BiomeMaterialsPanel::save_for_world(
        const world_editor::terrain::Heightfield& terrain,
        const world_editor::terrain::Settings& settings,
        const world_editor::biome::HydrologyLayer* hydrology,
        const std::filesystem::path& directory
    ) const {
        if (!active_) {
            std::filesystem::remove(
                biome::biome_file_path(directory)
            );

            return;
        }

        const std::uint64_t fingerprint =
            biome::biome_source_fingerprint(
                terrain,
                hydrology
            );

        if (fingerprint != sourceFingerprint_) {
            throw std::runtime_error(
                "Terrain or Hydrology changed. "
                "Regenerate Biomes before saving."
            );
        }

        biome::save_biomes(
            *active_,
            settings,
            sourceFingerprint_,
            directory
        );
    }

    bool BiomeMaterialsPanel::restore_from_world(
        const world_editor::terrain::Settings& settings,
        viewport::TerrainViewport& viewport,
        const std::filesystem::path& directory,
        const world_editor::terrain::Heightfield* terrain
    ) {
        active_.reset();
        sourceFingerprint_ = 0;

        if (!std::filesystem::exists(
            biome::biome_file_path(directory)
        )) {
            status_.clear();
            hasError_ = false;
            return false;
        }

        auto saved = biome::load_biomes(
            settings,
            directory
        );

        if (terrain) {
            const std::uint64_t fingerprint =
                biome::biome_source_fingerprint(
                    *terrain,
                    viewport.hydrology()
                );

            if (fingerprint != saved.sourceFingerprint) {
                throw std::runtime_error(
                    "Saved Biomes do not match "
                    "Terrain or Hydrology"
                );
            }
        }

        viewport.set_biomes(saved.layer);

        sourceFingerprint_ = saved.sourceFingerprint;
        pending_ = saved.layer.settings;
        active_ = std::move(saved.layer);

        status_ = "Biome materials restored";
        hasError_ = false;

        return true;
    }

}
