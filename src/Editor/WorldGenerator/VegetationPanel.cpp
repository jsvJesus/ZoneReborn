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
#include <unordered_map>
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
            "Any",
            "Grassland",
            "Forest",
            "Sandy",
            "Rocky",
            "Wetland"
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

    void VegetationPanel::invalidate(
        viewport::TerrainViewport& viewport
    ) {
        active_.reset();
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

    void VegetationPanel::refresh_catalog(
        viewport::TerrainViewport& viewport
    ) {
        auto updated = vegetation::scan_vegetation_catalog(
            editor::paths::packs_directory()
        );

        // Preserve manual category, habitat, weight and
        // enabled state when the catalog is rescanned.
        std::unordered_map<
            std::string,
            vegetation::VegetationCatalogEntry
        > previous;

        previous.reserve(catalog_.entries.size());

        for (const auto& entry : catalog_.entries) {
            previous.emplace(
                entry.relativePath,
                entry
            );
        }

        for (auto& entry : updated.entries) {
            const auto found = previous.find(
                entry.relativePath
            );

            if (found == previous.end()) {
                continue;
            }

            entry.kind = found->second.kind;
            entry.habitat = found->second.habitat;
            entry.weight = found->second.weight;
            entry.enabled = found->second.enabled;
        }

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
        viewport::TerrainViewport& viewport
    ) {
        if (!catalogInitialized_) {
            catalogInitialized_ = true;

            try {
                refresh_catalog(viewport);
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        if (active_ && !viewport.has_vegetation()) {
            active_.reset();
            counts_ = {};
            usedVariants_ = 0;

            status_ = "Vegetation invalidated. Regenerate.";
            hasError_ = false;
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

        constexpr std::uint32_t minInstances = 1000;
        constexpr std::uint32_t maxInstances = 500000;

        ImGui::SliderScalar(
            "Maximum Instances",
            ImGuiDataType_U32,
            &pending_.maxInstances,
            &minInstances,
            &maxInstances
        );

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

        if (ImGui::CollapsingHeader(
            "Asset Catalog",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            if (ImGui::Button("Rescan models/flora")) {
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
                    if (entry.enabled) {
                        entry.enabled = false;
                        catalogChanged = true;
                    }
                }
            }

            ImGui::EndDisabled();

            std::size_t lodCount = 0;
            std::size_t unclassifiedCount = 0;

            for (const auto& entry : catalog_.entries) {
                if (entry.lodVariant) {
                    ++lodCount;
                }

                if (entry.kind ==
                    vegetation::VegetationKind::Unclassified) {
                    ++unclassifiedCount;
                }
            }

            ImGui::Text(
                "Found .primitives: %zu",
                catalog_.entries.size()
            );

            ImGui::Text(
                "Enabled variants: %zu",
                catalog_.enabled_count()
            );

            ImGui::Text(
                "LOD files: %zu | Unclassified: %zu",
                lodCount,
                unclassifiedCount
            );

            ImGui::TextDisabled(
                "LOD variants are disabled by default."
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputText(
                "##asset_filter",
                assetFilter_,
                sizeof(assetFilter_)
            );

            const std::string filter =
                lowercase(assetFilter_);

            ImGui::BeginChild(
                "##vegetation_asset_list",
                ImVec2(0.0f, 265.0f),
                true
            );

            for (std::size_t i = 0;
                 i < catalog_.entries.size();
                 ++i) {
                auto& entry = catalog_.entries[i];

                if (!filter.empty() &&
                    lowercase(entry.relativePath).find(
                        filter
                    ) == std::string::npos) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(i));

                if (ImGui::Checkbox(
                    "##enabled",
                    &entry.enabled
                )) {
                    catalogChanged = true;
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
                }

                ImGui::SameLine();

                int habitat = static_cast<int>(
                    entry.habitat
                );

                ImGui::SetNextItemWidth(105.0f);

                if (ImGui::Combo(
                    "##habitat",
                    &habitat,
                    habitatNames,
                    IM_ARRAYSIZE(habitatNames)
                )) {
                    entry.habitat =
                        static_cast<vegetation::VegetationHabitat>(
                            habitat
                        );

                    catalogChanged = true;
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
                }

                if (entry.lodVariant) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("LOD");
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            ImGui::EndChild();

            ImGui::TextDisabled(
                "Columns: Enabled / Kind / Habitat / Weight"
            );

            if (catalogChanged) {
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

        ImGui::BeginDisabled(
            !terrain.valid() ||
            !biomes ||
            viewport.is_streaming() ||
            catalog_.enabled_count() == 0
        );

        if (ImGui::Button(
            "Generate Vegetation",
            ImVec2(-1.0f, 42.0f)
        )) {
            try {
                auto settings = pending_;

                if (useLayoutSeed_ && layout) {
                    settings.seed =
                        layout->settings.seeds.vegetation;
                }

                const std::uint64_t currentFingerprint =
                    world_editor::biome::biome_source_fingerprint(
                        terrain,
                        viewport.hydrology()
                    );

                if (currentFingerprint !=
                    biomeSourceFingerprint) {
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
                        catalog_
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
                "Vegetation persistence is a separate step."
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
            "Deferred by preview budget: %u",
            renderStats.skippedVariants
        );

        ImGui::Text(
            "GPU instances: %u",
            renderStats.renderedInstances
        );

        ImGui::Text(
            "Vegetation draw calls: %u",
            renderStats.drawCalls
        );

        ImGui::TextDisabled(
            "Real .primitives geometry, diagnostic materials"
        );

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
