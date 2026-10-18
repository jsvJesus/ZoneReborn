#include "Editor/WorldGenerator/VegetationPanel.h"

#include "Editor/Core/Paths.h"
#include "Editor/Viewport/TerrainViewport.h"

#include "WorldEditor/Biome/BiomeStorage.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace editor::world_generator {

    namespace {

        namespace vegetation = world_editor::vegetation;

        void validate_assets(
            const vegetation::VegetationLayer& layer
        ) {
            const auto packs =
                editor::paths::packs_directory();

            std::array<bool,
                static_cast<std::size_t>(
                    vegetation::VegetationAsset::Count
                )
            > checked{};

            for (const auto& instance : layer.instances) {
                const std::size_t id =
                    static_cast<std::size_t>(instance.asset);

                if (id >= checked.size()) {
                    throw std::runtime_error(
                        "Invalid vegetation asset ID"
                    );
                }

                if (checked[id]) {
                    continue;
                }

                checked[id] = true;

                const auto relative =
                    vegetation::asset_relative_path(
                        instance.asset
                    );

                const auto path =
                    packs / std::filesystem::path(relative);

                if (!std::filesystem::is_regular_file(path)) {
                    throw std::runtime_error(
                        "Missing vegetation asset: " +
                        path.string()
                    );
                }
            }
        }

    }

    void VegetationPanel::clear(
        viewport::TerrainViewport& viewport
    ) {
        active_.reset();
        counts_ = {};
        status_.clear();
        hasError_ = false;

        viewport.set_vegetation(nullptr);
    }

    void VegetationPanel::draw(
        const world_editor::terrain::Heightfield& terrain,
        const world_editor::biome::BiomeLayer* biomes,
        std::uint64_t biomeSourceFingerprint,
        viewport::TerrainViewport& viewport
    ) {
        if (active_ && !viewport.has_vegetation()) {
            active_.reset();
            counts_ = {};

            status_ = "Vegetation invalidated. Regenerate.";
            hasError_ = false;
        }
        
        ImGui::Begin("Vegetation Generator");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "VEGETATION GENERATOR"
        );

        ImGui::TextDisabled(
            "Grass / Bushes / Trees / Wetlands"
        );

        ImGui::Separator();

        ImGui::Checkbox(
            "Use Map Layout VegetationSeed",
            &useLayoutSeed_
        );

        const auto* layout = viewport.map_layout();

        if (useLayoutSeed_ && layout) {
            ImGui::Text(
                "Layout VegetationSeed: %u",
                layout->settings.seeds.vegetation
            );
        } else {
            ImGui::InputScalar(
                "Vegetation Seed",
                ImGuiDataType_U32,
                &pending_.seed
            );
        }

        ImGui::SliderFloat(
            "Grass Density",
            &pending_.grassDensity,
            0.0f,
            1.0f,
            "%.2f"
        );

        ImGui::SliderFloat(
            "Bush Density",
            &pending_.bushDensity,
            0.0f,
            1.0f,
            "%.2f"
        );

        ImGui::SliderFloat(
            "Tree Density",
            &pending_.treeDensity,
            0.0f,
            1.0f,
            "%.2f"
        );

        ImGui::SliderFloat(
            "Road Clearance",
            &pending_.roadClearanceMeters,
            0.0f,
            32.0f,
            "%.1f m"
        );

        ImGui::SliderFloat(
            "POI Clearance",
            &pending_.poiClearanceMeters,
            0.0f,
            48.0f,
            "%.1f m"
        );

        ImGui::SliderFloat(
            "Exit Clearance",
            &pending_.exitClearanceMeters,
            0.0f,
            48.0f,
            "%.1f m"
        );

        constexpr std::uint32_t minimumInstances = 1000;
        constexpr std::uint32_t maximumInstances = 500000;

        ImGui::SliderScalar(
            "Maximum Instances",
            ImGuiDataType_U32,
            &pending_.maxInstances,
            &minimumInstances,
            &maximumInstances
        );

        ImGui::Checkbox(
            "Show Placement Preview",
            &showPreview_
        );

        viewport.set_vegetation_visible(showPreview_);

        ImGui::Spacing();

        if (!biomes) {
            ImGui::TextWrapped(
                "Generate Biomes first. Vegetation needs "
                "an active Biome Layer."
            );
        }

        if (viewport.is_streaming()) {
            ImGui::TextWrapped(
                "Vegetation generation is disabled "
                "while streaming the world."
            );
        }

        ImGui::BeginDisabled(
            !terrain.valid() ||
            !biomes ||
            viewport.is_streaming()
        );

        if (ImGui::Button(
            "Generate Vegetation",
            ImVec2(-1.0f, 42.0f)
        )) {
            try {
                vegetation::VegetationSettings settings =
                    pending_;

                if (useLayoutSeed_ && layout) {
                    settings.seed =
                        layout->settings.seeds.vegetation;
                }

                const auto currentFingerprint =
                    world_editor::biome::biome_source_fingerprint(
                    terrain,
                    viewport.hydrology()
                );

                if (currentFingerprint != biomeSourceFingerprint) {
                    throw std::runtime_error(
                        "Terrain or Hydrology changed. "
                        "Regenerate Biomes before Vegetation."
                    );
                }

                auto generated =
                    vegetation::generate_vegetation(
                        terrain,
                        *biomes,
                        viewport.hydrology(),
                        layout,
                        settings
                    );

                validate_assets(generated);

                viewport.set_vegetation(&generated);

                counts_ = {};

                for (const auto& instance :
                     generated.instances) {
                    const auto kind = static_cast<
                        std::size_t
                    >(instance.kind);

                    if (kind < counts_.size()) {
                        ++counts_[kind];
                    }
                }

                active_ = std::move(generated);

                status_ = "Vegetation placement generated";
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
                "Active Seed: %u",
                active_->settings.seed
            );

            ImGui::Text(
                "Grass anchors: %zu",
                counts_[0]
            );

            ImGui::Text(
                "Bush instances: %zu",
                counts_[1]
            );

            ImGui::Text(
                "Tree instances: %zu",
                counts_[2]
            );

            ImGui::Text(
                "Total: %zu",
                active_->instances.size()
            );

            ImGui::TextDisabled(
                "Preview: placement markers, not 3D models"
            );

            ImGui::TextDisabled(
                "This v1 preview is not saved with World yet"
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
