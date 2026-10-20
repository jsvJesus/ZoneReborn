#include "Editor/WorldGenerator/VegetationPanel.h"

#include "Editor/Core/Paths.h"
#include "Editor/Viewport/TerrainViewport.h"

#include "WorldEditor/Biome/BiomeStorage.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace editor::world_generator {

    namespace {

        namespace vegetation = world_editor::vegetation;

        constexpr const char* kindNames[] = {
            "Grass",
            "Bush",
            "Tree",
            "Unclassified"
        };

        constexpr const char* habitatNames[] = {
            "All Biomes",
            "Grassland",
            "Forest",
            "Sandy",
            "Rocky",
            "Swamp / Wetland"
        };

        constexpr const char* categoryNames[] = {
            "All Categories",
            "Grass",
            "Bushes",
            "Trees",
            "Unclassified"
        };

        constexpr const char* sourceNames[] = {
            "SpeedTree (.spt)",
            "Flora (.primitives)"
        };

        [[nodiscard]] std::string lowercase(
            std::string value
        ) {
            for (char& ch : value) {
                ch = static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(ch)
                    )
                );
            }

            return value;
        }

        void verify_used_assets(
            const vegetation::VegetationLayer& layer
        ) {
            const auto packs =
                editor::paths::packs_directory();

            std::vector<bool> checked(
                layer.assetPaths.size(),
                false
            );

            for (const auto& instance : layer.instances) {
                if (instance.assetIndex >= checked.size()) {
                    throw std::runtime_error(
                        "Invalid generated vegetation asset ID"
                    );
                }

                if (checked[instance.assetIndex]) {
                    continue;
                }

                checked[instance.assetIndex] = true;

                const auto path =
                    packs / std::filesystem::path(
                        layer.asset_path(instance)
                    );

                if (!std::filesystem::is_regular_file(path)) {
                    throw std::runtime_error(
                        "Vegetation asset no longer exists: " +
                        path.string()
                    );
                }
            }
        }

    }

    VegetationPanel::~VegetationPanel() {
        if (!preferencesDirty_) {
            return;
        }

        try {
            save_vegetation_preferences(
                preferences_, editor::paths::vegetation_settings_path()
            );
        } catch (...) {
        }
    }

    void VegetationPanel::persist_preferences() {
        try {
            save_vegetation_preferences(
                preferences_, editor::paths::vegetation_settings_path()
            );
            preferencesDirty_ = false;
            preferencesSaveFailed_ = false;
        } catch (const std::exception& error) {
            status_ = error.what();
            hasError_ = true;
            preferencesSaveFailed_ = true;
        }
    }

    void VegetationPanel::invalidate(
        viewport::TerrainViewport& viewport
    ) {
        active_.reset();
        sourceRevisions_ = {};
        counts_ = {};
        usedVariants_ = 0;

        viewport.set_vegetation(nullptr);
    }

    void VegetationPanel::clear(
        viewport::TerrainViewport& viewport
    ) {
        invalidate(viewport);

        status_.clear();
        hasError_ = false;
    }

    void VegetationPanel::synchronize(
        viewport::TerrainViewport& viewport,
        const world_editor::world_generator::GenerationSeeds& seeds
    ) {
        pending_.seed = seeds.vegetation;
        pending_.decorationSeed = seeds.decoration;

        if (active_ &&
            (!viewport.has_vegetation() || !viewport.has_biomes() ||
                sourceRevisions_ != viewport.layer_revisions() ||
                active_->settings != pending_)) {
            invalidate(viewport);
            status_ = "Vegetation dependencies changed. Regenerate Vegetation.";
            hasError_ = false;
        }
    }

    void VegetationPanel::refresh_catalog(
        viewport::TerrainViewport& viewport
    ) {
        auto updated = vegetation::scan_vegetation_catalog(
            editor::paths::packs_directory()
        );

        preferences_.apply(updated);

        catalog_ = std::move(updated);
        catalogInitialized_ = true;

        invalidate(viewport);

        status_ = "Vegetation asset catalog scanned";
        hasError_ = false;
    }

    void VegetationPanel::draw(
        const world_editor::terrain::Heightfield& terrain,
        const world_editor::biome::BiomeLayer* biomes,
        std::uint64_t biomeSourceFingerprint,
        viewport::TerrainViewport& viewport,
        const world_editor::world_generator::GenerationSeeds& seeds
    ) {
        synchronize(viewport, seeds);

        if (!catalogInitialized_) {
            catalogInitialized_ = true;

            std::string loadError;

            try {
                if (auto saved = load_vegetation_preferences(
                    editor::paths::vegetation_settings_path()
                )) {
                    preferences_ = std::move(*saved);
                    selectedSource_ = preferences_.source ==
                        vegetation::VegetationAssetSource::SpeedTree ? 0 : 1;
                }
            } catch (const std::exception& error) {
                loadError = error.what();
            }

            try {
                refresh_catalog(viewport);

                if (!loadError.empty()) {
                    status_ = loadError;
                    hasError_ = true;
                }
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        ImGui::Begin("Vegetation Generator");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "VEGETATION GENERATOR"
        );

        ImGui::TextDisabled(
            "Procedural placement / Dynamic Asset Catalog"
        );

        ImGui::Separator();

        const auto* layout = viewport.map_layout();

        ImGui::Text("Map VegetationSeed: %u", seeds.vegetation);
        ImGui::Text("Map DecorationSeed: %u", seeds.decoration);
        ImGui::TextDisabled("Edit seeds in Map Layout / Generation Seeds.");

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

        constexpr std::uint32_t minInstances = 1000;
        constexpr std::uint32_t maxInstances = 500000;

        ImGui::SliderScalar(
            "Maximum Instances",
            ImGuiDataType_U32,
            &pending_.maxInstances,
            &minInstances,
            &maxInstances
        );

        synchronize(viewport, seeds);

        ImGui::Checkbox(
            "Show Placement Preview",
            &showPreview_
        );

        viewport.set_vegetation_visible(
            showPreview_
        );

        ImGui::Checkbox(
            "Show 3D Geometry Preview",
            &showGeometry_
        );

        viewport.set_vegetation_geometry_visible(
            showGeometry_
        );

        ImGui::Separator();

        if (ImGui::Combo(
            "Model Source",
            &selectedSource_,
            sourceNames,
            IM_ARRAYSIZE(sourceNames)
        )) {
            invalidate(viewport);
            status_ = "Model source changed. Regenerate vegetation.";
            hasError_ = false;
        }

        const auto selectedSource = selectedSource_ == 0
            ? vegetation::VegetationAssetSource::SpeedTree
            : vegetation::VegetationAssetSource::Flora;

        if (preferences_.source != selectedSource) {
            preferences_.source = selectedSource;
            preferencesDirty_ = true;
            preferencesSaveFailed_ = false;
        }

        if (ImGui::CollapsingHeader(
            "Asset Catalog",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            if (ImGui::Button("Rescan Flora + SpeedTree")) {
                try {
                    refresh_catalog(viewport);
                } catch (const std::exception& error) {
                    status_ = error.what();
                    hasError_ = true;
                }
            }

            bool catalogChanged = false;

            ImGui::SameLine();
            ImGui::BeginDisabled(catalog_.entries.empty());

            if (ImGui::Button("Uncheck All")) {
                for (auto& entry : catalog_.entries) {
                    if (entry.source == selectedSource &&
                        entry.enabled) {
                        entry.enabled = false;
                        preferences_.remember(entry);
                        catalogChanged = true;
                    }
                }
            }

            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::BeginDisabled(catalog_.entries.empty());

            if (ImGui::Button("Enable Classified")) {
                const auto selectedMask = selectedHabitat_ == 0
                    ? vegetation::allVegetationBiomes
                    : vegetation::vegetation_biome_mask(
                        static_cast<vegetation::VegetationHabitat>(
                            selectedHabitat_
                        )
                    );

                for (auto& entry : catalog_.entries) {
                    if (entry.source != selectedSource ||
                        entry.lodVariant ||
                        entry.kind == vegetation::VegetationKind::Unclassified ||
                        entry.weight <= 0.0f) {
                        continue;
                    }

                    if (!entry.enabled) {
                        entry.biomeMask = selectedHabitat_ == 0
                            ? (entry.biomeMask == 0
                                ? selectedMask
                                : entry.biomeMask)
                            : selectedMask;
                        entry.enabled = true;
                        preferences_.remember(entry);
                        catalogChanged = true;
                    } else if (selectedHabitat_ != 0 &&
                        (entry.biomeMask & selectedMask) == 0) {
                        entry.biomeMask |= selectedMask;
                        preferences_.remember(entry);
                        catalogChanged = true;
                    }
                }
            }

            ImGui::EndDisabled();

            std::size_t modelCount = 0;
            std::size_t unclassifiedCount = 0;
            std::size_t checkedCount = 0;

            for (const auto& entry : catalog_.entries) {
                if (entry.source != selectedSource ||
                    entry.lodVariant) {
                    continue;
                }

                ++modelCount;

                if (entry.kind ==
                    vegetation::VegetationKind::Unclassified) {
                    ++unclassifiedCount;
                }

                if (entry.enabled && entry.biomeMask != 0) {
                    ++checkedCount;
                }
            }

            ImGui::Text(
                "Found models: %zu",
                modelCount
            );

            ImGui::Text(
                "Enabled variants: %zu",
                catalog_.enabled_count(selectedSource)
            );

            ImGui::Text(
                "Checked models: %zu",
                checkedCount
            );

            ImGui::Text(
                "Unclassified: %zu",
                unclassifiedCount
            );

            ImGui::SeparatorText("Biome Asset Selection");
            ImGui::Combo(
                "Biome Mask",
                &selectedHabitat_,
                habitatNames,
                IM_ARRAYSIZE(habitatNames)
            );
            ImGui::Combo(
                "Category",
                &selectedKind_,
                categoryNames,
                IM_ARRAYSIZE(categoryNames)
            );

            ImGui::TextDisabled(
                selectedHabitat_ == 0
                    ? "Check models to enable their assigned biome masks."
                    : "Check models to place them in this biome mask."
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputText(
                "##asset_filter",
                assetFilter_,
                sizeof(assetFilter_)
            );

            const std::string filter =
                lowercase(assetFilter_);

            const auto selectedMask = selectedHabitat_ == 0
                ? vegetation::allVegetationBiomes
                : vegetation::vegetation_biome_mask(
                    static_cast<vegetation::VegetationHabitat>(
                        selectedHabitat_
                    )
                );

            ImGui::BeginChild(
                "##vegetation_asset_list",
                ImVec2(0.0f, 265.0f),
                true
            );

            for (std::size_t i = 0;
                 i < catalog_.entries.size();
                 ++i) {
                auto& entry = catalog_.entries[i];

                if (entry.source != selectedSource ||
                    entry.lodVariant) {
                    continue;
                }

                if (selectedKind_ != 0 &&
                    static_cast<int>(entry.kind) !=
                        selectedKind_ - 1) {
                    continue;
                }

                if (!filter.empty() &&
                    lowercase(entry.relativePath).find(
                        filter
                    ) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(i));

                bool checked = entry.enabled &&
                    (selectedHabitat_ == 0 ||
                        (entry.biomeMask & selectedMask) != 0);

                if (ImGui::Checkbox("##enabled", &checked)) {
                    if (selectedHabitat_ == 0) {
                        entry.enabled = checked;

                        if (checked && entry.biomeMask == 0) {
                            entry.biomeMask =
                                vegetation::allVegetationBiomes;
                        }
                    } else if (checked) {
                        if (!entry.enabled) {
                            entry.biomeMask = 0;
                        }

                        entry.biomeMask |= selectedMask;
                        entry.enabled = true;
                    } else {
                        entry.biomeMask &= static_cast<std::uint8_t>(
                            ~selectedMask
                        );
                        entry.enabled = entry.biomeMask != 0;
                    }

                    catalogChanged = true;
                    preferences_.remember(entry);
                }

                ImGui::SameLine();

                ImGui::TextUnformatted(
                    entry.relativePath.c_str()
                );

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "%s",
                        entry.relativePath.c_str()
                    );
                }

                int kind = static_cast<int>(
                    entry.kind
                );

                ImGui::SetNextItemWidth(105.0f);

                if (ImGui::Combo(
                    "##kind",
                    &kind,
                    kindNames,
                    IM_ARRAYSIZE(kindNames)
                )) {
                    entry.kind =
                        static_cast<vegetation::VegetationKind>(
                            kind
                        );

                    catalogChanged = true;
                    preferences_.remember(entry);
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(90.0f);

                if (ImGui::DragFloat(
                    "##weight",
                    &entry.weight,
                    0.05f,
                    0.0f,
                    20.0f,
                    "%.2f"
                )) {
                    catalogChanged = true;
                    preferences_.remember(entry);
                }

                if (selectedHabitat_ == 0) {
                    char masks[] = "-----";
                    constexpr char labels[] = "GFSRW";

                    for (unsigned bit = 0; bit < 5; ++bit) {
                        if ((entry.biomeMask & (1u << bit)) != 0) {
                            masks[bit] = labels[bit];
                        }
                    }

                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", masks);

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(
                            "Grassland / Forest / Sandy / Rocky / Wetland"
                        );
                    }
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            ImGui::EndChild();

            ImGui::TextDisabled(
                "Columns: Enabled / Kind / Weight / Biome masks"
            );

            if (catalogChanged) {
                preferencesDirty_ = true;
                preferencesSaveFailed_ = false;
                invalidate(viewport);

                status_ =
                    "Asset catalog changed. Regenerate vegetation.";

                hasError_ = false;
            }
        }

        ImGui::Spacing();

        if (!biomes) {
            ImGui::TextWrapped(
                "Generate Biomes before Vegetation."
            );
        }

        if (viewport.is_streaming()) {
            ImGui::TextWrapped(
                "Generation is disabled during streaming."
            );
        }

        if (catalog_.enabled_count(selectedSource) == 0) {
            ImGui::TextWrapped(
                "No usable models selected. Click Enable Classified, "
                "or set checked models to Grass, Bush or Tree "
                "with a weight above zero."
            );
        }

        ImGui::BeginDisabled(
            !terrain.valid() ||
            !biomes ||
            viewport.is_streaming() ||
            catalog_.enabled_count(selectedSource) == 0
        );

        if (ImGui::Button(
            "Generate Vegetation",
            ImVec2(-1.0f, 42.0f)
        )) {
            try {
                auto settings = pending_;

                const std::uint64_t currentFingerprint =
                    world_editor::biome::biome_source_fingerprint(
                        terrain,
                        viewport.hydrology()
                    );

                if (currentFingerprint !=
                    biomeSourceFingerprint) {
                    invalidate(viewport);
                    viewport.clear_biomes();
                    throw std::runtime_error(
                        "Terrain or Hydrology changed. "
                        "Regenerate Biomes first."
                    );
                }

                auto generated =
                    vegetation::generate_vegetation(
                        terrain,
                        *biomes,
                        viewport.hydrology(),
                        layout,
                        settings,
                        catalog_.for_source(selectedSource)
                    );

                verify_used_assets(generated);

                // A copied snapshot is retained by the viewport.
                viewport.set_vegetation(&generated);

                counts_ = {};
                usedVariants_ = 0;

                std::vector<bool> used(
                    generated.assetPaths.size(),
                    false
                );

                for (const auto& instance :
                     generated.instances) {
                    const std::size_t kind =
                        static_cast<std::size_t>(
                            instance.kind
                        );

                    if (kind < counts_.size()) {
                        ++counts_[kind];
                    }

                    used[instance.assetIndex] = true;
                }

                usedVariants_ = static_cast<std::size_t>(
                    std::count(
                        used.begin(),
                        used.end(),
                        true
                    )
                );

                active_ = std::move(generated);
                sourceRevisions_ = viewport.layer_revisions();

                status_ = "Vegetation generated from catalog";
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
                "Grass instances: %zu",
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
                "Total instances: %zu",
                active_->instances.size()
            );

            ImGui::Text(
                "Different asset variants used: %zu",
                usedVariants_
            );

            ImGui::TextDisabled(
                "Preview uses markers, not 3D geometry."
            );

            ImGui::TextDisabled(
                "Model selections are saved; generated placement is separate."
            );
        }

        ImGui::Separator();

        ImGui::TextColored(
        ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
        "3D GEOMETRY IMPORT"
        );

        const auto& renderStats =
            viewport.vegetation_render_stats();

        ImGui::Text(
            "Imported model variants: %u",
            renderStats.loadedVariants
        );

        ImGui::Text(
        "Material groups: %u",
            renderStats.loadedMaterialGroups
        );

        ImGui::Text(
            "Unique DDS textures: %u",
            renderStats.loadedTextures
        );

        ImGui::Text(
            "Unsupported / failed: %u",
            renderStats.failedVariants
        );

        if (!renderStats.failedPaths.empty() && ImGui::TreeNode("Import errors"))
            {
            for (const auto& error : renderStats.failedPaths) {
                ImGui::TextWrapped(
                    "%s",
                    error.c_str()
                );
            }

            ImGui::TreePop();
        }

        ImGui::Text(
            "GPU instances: %u",
            renderStats.renderedInstances
        );

        ImGui::Text(
            "Vegetation draw calls: %u",
            renderStats.drawCalls
        );

        ImGui::TextDisabled(
            "Flora .primitives / SpeedTree .spt + .ctree"
        );

        if (preferencesDirty_ &&
            !preferencesSaveFailed_ &&
            !ImGui::IsAnyItemActive()) {
            persist_preferences();
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
