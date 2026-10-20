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
        const world_editor::terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport,
        const world_editor::world_generator::GenerationSeeds& seeds
    ) {
        synchronize(viewport, seeds, activeSettings);

        ImGui::Begin("Biome Materials");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "BIOME SETTINGS"
        );

        ImGui::TextDisabled(
            "Grass / Soil / Sand / Rock / Marsh"
        );

        ImGui::Separator();

        ImGui::Text(
            "Map BiomeSeed: %u",
            seeds.biome
        );
        ImGui::TextDisabled("Edit seeds in Map Layout / Generation Seeds.");

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

        synchronize(viewport, seeds, activeSettings);

        if (viewport.is_streaming()) {
            ImGui::TextWrapped(
                "Streaming mode: use saved Biome data. "
                "Switch to Load World to regenerate."
            );
        } else {
            ImGui::TextDisabled(
                "Select Biomes in World Generator to apply these settings."
            );
        }

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

    void BiomeMaterialsPanel::generate(
        const world_editor::terrain::Heightfield& terrain,
        const world_editor::terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport,
        const world_editor::world_generator::GenerationSeeds& seeds
    ) {
        synchronize(viewport, seeds, activeSettings);

        if (!terrain.valid() || viewport.is_streaming() ||
            activeSettings.seed != seeds.terrain) {
            throw std::runtime_error("Biome generation requires matching Terrain");
        }

        if (biome::terrain_source_fingerprint(terrain) !=
            viewport.terrain_fingerprint()) {
            clear(viewport);
            throw std::runtime_error("Terrain heights changed. Apply Terrain first.");
        }

        auto generated = biome::generate_biomes(
            terrain,
            viewport.hydrology(),
            pending_
        );

        viewport.set_biomes(generated);

        sourceFingerprint_ = biome::biome_source_fingerprint(
            terrain,
            viewport.hydrology()
        );

        active_ = std::move(generated);
        sourceRevisions_ = viewport.layer_revisions();
        status_ = "Biome materials applied";
        hasError_ = false;
    }

    void BiomeMaterialsPanel::clear(viewport::TerrainViewport& viewport) {
        active_.reset();
        sourceRevisions_ = {};
        sourceFingerprint_ = 0;
        viewport.clear_biomes();
        status_.clear();
        hasError_ = false;
    }

    void BiomeMaterialsPanel::synchronize(
        viewport::TerrainViewport& viewport,
        const world_editor::world_generator::GenerationSeeds& seeds,
        const world_editor::terrain::Settings& activeSettings
    ) {
        pending_.seed = seeds.biome;

        if (active_ &&
            (!viewport.has_biomes() ||
                sourceRevisions_ != viewport.layer_revisions() ||
                active_->settings != pending_ ||
                activeSettings.seed != seeds.terrain)) {
            clear(viewport);
            status_ = "Biome dependencies changed. Regenerate Biomes.";
        }
    }

    void BiomeMaterialsPanel::validate_for_save(
        const world_editor::terrain::Heightfield& terrain,
        const world_editor::biome::HydrologyLayer* hydrology
    ) const {
        if (active_ && biome::biome_source_fingerprint(terrain, hydrology) != sourceFingerprint_) {
            throw std::runtime_error(
                "Terrain or Hydrology changed. Regenerate Biomes before saving."
            );
        }
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

        validate_for_save(terrain, hydrology);

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
        const world_editor::terrain::Heightfield& terrain
    ) {
        clear(viewport);

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

        const std::uint64_t fingerprint =
            biome::biome_source_fingerprint(
                terrain,
                viewport.hydrology()
            );

        if (fingerprint != saved.sourceFingerprint) {
            throw std::runtime_error(
                "Saved Biomes do not match "
                "Terrain or Hydrology"
            );
        }

        viewport.set_biomes(saved.layer);

        sourceFingerprint_ = saved.sourceFingerprint;
        pending_ = saved.layer.settings;
        active_ = std::move(saved.layer);
        sourceRevisions_ = viewport.layer_revisions();

        status_ = "Biome materials restored";
        hasError_ = false;

        return true;
    }

}
