#include "Editor/WorldGenerator/MapLayoutPanel.h"

#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/WorldGenerator/WorldStoragePanel.h"

#include "WorldEditor/Biome/BiomeStorage.h"
#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Biome/HydrologyStorage.h"
#include "WorldEditor/Terrain/TerrainCorrection.h"
#include "WorldEditor/Terrain/TerrainWorldStorage.h"
#include "WorldEditor/WorldGenerator/MapLayoutStorage.h"

#include "../src/Externals/DearImGui/imgui.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace editor::world_generator {

    namespace {

        namespace wg = world_editor::world_generator;
        namespace terrain = world_editor::terrain;
        namespace settlement = world_editor::settlement;

        [[nodiscard]] std::filesystem::path layout_path(
            const std::filesystem::path& directory
        ) {
            std::filesystem::path path = directory;
            path += L".zlayout";
            return path;
        }

        [[nodiscard]] std::filesystem::path hydrology_path(
            const std::filesystem::path& directory
        ) {
            std::filesystem::path path = directory;
            path += L".zhydro";
            return path;
        }

        [[nodiscard]] std::filesystem::path correction_path(
            const std::filesystem::path& directory
        ) {
            std::filesystem::path path = directory;
            path += L".zbase";
            return path;
        }

        [[nodiscard]] bool same_terrain(
            const terrain::Settings& a,
            const terrain::Settings& b
        ) noexcept {
            return a.samplesX == b.samplesX &&
                a.samplesZ == b.samplesZ &&
                a.seed == b.seed &&
                a.originX == b.originX &&
                a.originZ == b.originZ &&
                a.spacingMeters == b.spacingMeters &&
                a.baseHeightMeters == b.baseHeightMeters &&
                a.reliefMeters == b.reliefMeters &&
                a.macroScaleMeters == b.macroScaleMeters &&
                a.detailScaleMeters == b.detailScaleMeters;
        }

        [[nodiscard]] const char* poi_name(
            const settlement::POIType type
        ) {
            switch (type) {
            case settlement::POIType::Checkpoint:
                return "Checkpoint";
            case settlement::POIType::Farm:
                return "Farm";
            case settlement::POIType::Village:
                return "Village";
            case settlement::POIType::Industrial:
                return "Industrial";
            case settlement::POIType::Town:
                return "Town";
            }

            return "Unknown";
        }

        [[nodiscard]] int selected_map_size(
            const wg::MapSize size
        ) noexcept {
            switch (size) {
            case wg::MapSize::Small:
                return 0;
            case wg::MapSize::Medium:
                return 1;
            case wg::MapSize::Large:
                return 2;
            case wg::MapSize::ExtraLarge:
                return 3;
            }

            return 2;
        }

        void apply_preset(
            wg::MapGenerationSettings& settings,
            const int preset
        ) {
            switch (preset) {
            case 1:
                settings.allowedTypes = {
                    settlement::POIType::Checkpoint,
                    settlement::POIType::Farm,
                    settlement::POIType::Village
                };
                break;

            case 2:
                settings.allowedTypes = {
                    settlement::POIType::Checkpoint,
                    settlement::POIType::Farm
                };
                break;

            case 3:
                settings.allowedTypes = {
                    settlement::POIType::Checkpoint,
                    settlement::POIType::Industrial,
                    settlement::POIType::Town
                };
                break;

            default:
                settings.allowedTypes.clear();
                break;
            }
        }

    }

    MapLayoutPanel::MapLayoutPanel() {
        settings_.mapSize = wg::MapSize::Large;
    }

    void MapLayoutPanel::save_hydrology_for_world(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& settings,
        const std::filesystem::path& directory
    ) const {
        if (!hydrology_) {
            std::filesystem::remove(hydrology_path(directory));
            return;
        }

        if (!sourceHydrologyTerrain_ ||
            !same_terrain(*sourceHydrologyTerrain_, settings) ||
            world_editor::biome::terrain_source_fingerprint(heightfield) !=
                hydrologyTerrainFingerprint_) {
            throw std::runtime_error(
                "Hydrology Terrain settings mismatch"
            );
            }

        world_editor::biome::save_hydrology(
            *hydrology_,
            heightfield,
            settings,
            hydrology_path(directory)
        );
    }

    bool MapLayoutPanel::restore_hydrology(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& settings,
        viewport::TerrainViewport& viewport,
        const std::filesystem::path& directory
    ) {
        const auto path = hydrology_path(directory);
        const auto basePath = correction_path(directory);

        baseTerrain_.reset();
        correctedTerrainRevision_ = 0;

        if (std::filesystem::exists(basePath)) {
            try {
                baseTerrain_ = terrain::load_correction_base(
                    heightfield,
                    basePath
                );
                correctedTerrainRevision_ = viewport.layer_revisions().terrain;
            } catch (const std::exception&) {
                baseTerrain_.reset();
            }
        }

        if (!std::filesystem::exists(path)) {
            return false;
        }

        auto restored = world_editor::biome::load_hydrology(
            heightfield,
            settings,
            path
        );

        viewport.set_hydrology(&restored);

        hydrologySettings_ = restored.settings;
        settings_.seeds.water = restored.settings.seed;

        hydrology_ = std::move(restored);
        sourceHydrologyTerrain_ = settings;
        hydrologySource_ = viewport.layer_revisions();
        hydrologyTerrainFingerprint_ = viewport.terrain_fingerprint();

        return true;
    }

    void MapLayoutPanel::save_layout_for_world(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& settings,
        viewport::TerrainViewport& viewport,
        const std::filesystem::path& directory
    ) {
        validate_sources(heightfield, settings, viewport);

        if (!layout_) {
            std::filesystem::remove(layout_path(directory));
            std::filesystem::remove(correction_path(directory));
            return;
        }

        if (!sourceTerrain_ ||
            !same_terrain(*sourceTerrain_, settings)) {
            throw std::runtime_error(
                "Map Layout Terrain settings changed. Regenerate Layout."
            );
        }

        if (baseTerrain_) {
            terrain::save_correction_base(
                *baseTerrain_,
                heightfield,
                correction_path(directory)
            );
        } else {
            std::filesystem::remove(correction_path(directory));
        }

        wg::save_map_layout(
            *layout_,
            *sourceTerrain_,
            layout_path(directory)
        );
    }

    bool MapLayoutPanel::restore_layout_from_world(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& settings,
        viewport::TerrainViewport& viewport,
        const std::filesystem::path& directory
    ) {
        if (!std::filesystem::exists(layout_path(directory))) {
            return false;
        }

        auto loaded = wg::load_map_layout(
            layout_path(directory),
            settings,
            viewport.source_fingerprint()
        );

        if (viewport.hydrology() &&
            viewport.hydrology()->settings.seed !=
                loaded.settings.seeds.water) {
            throw std::runtime_error(
                "Load matching Hydrology before Map Layout."
            );
        }

        std::optional<terrain::Heightfield> restoredBase;

        if (std::filesystem::exists(correction_path(directory))) {
            restoredBase = terrain::load_correction_base(
                heightfield,
                correction_path(directory)
            );
        }

        viewport.set_map_layout(&loaded);

        layout_ = std::move(loaded);
        sourceTerrain_ = settings;
        settings_ = layout_->settings;
        layoutSource_ = viewport.layer_revisions();
        baseTerrain_ = std::move(restoredBase);
        correctedTerrainRevision_ = baseTerrain_
            ? viewport.layer_revisions().terrain : 0;

        status_ = "Map Layout loaded";
        hasError_ = false;
        return true;
    }

    void MapLayoutPanel::clear(
        viewport::TerrainViewport& viewport
    ) {
        if (correctedTerrainRevision_ != viewport.layer_revisions().terrain) {
            baseTerrain_.reset();
            correctedTerrainRevision_ = 0;
        }

        layout_.reset();
        sourceTerrain_.reset();
        hydrology_.reset();
        sourceHydrologyTerrain_.reset();
        layoutSource_ = {};
        hydrologySource_ = {};
        hydrologyTerrainFingerprint_ = 0;
        viewport.set_hydrology(nullptr);
    }

    void MapLayoutPanel::clear_layout(viewport::TerrainViewport& viewport) {
        layout_.reset();
        sourceTerrain_.reset();
        layoutSource_ = {};
        viewport.set_map_layout(nullptr);
    }

    void MapLayoutPanel::synchronize(
        viewport::TerrainViewport& viewport,
        const terrain::Settings& activeSettings
    ) {
        if (baseTerrain_ &&
            correctedTerrainRevision_ != viewport.layer_revisions().terrain) {
            baseTerrain_.reset();
            correctedTerrainRevision_ = 0;
        }

        hydrologySettings_.seed = settings_.seeds.water;
        const auto revisions = viewport.layer_revisions();

        if (hydrology_ &&
            (!viewport.hydrology() ||
                hydrologySource_.terrain != revisions.terrain ||
                hydrologySource_.hydrology != revisions.hydrology ||
                hydrology_->settings != hydrologySettings_ ||
                activeSettings.seed != settings_.seeds.terrain)) {
            clear(viewport);
            status_ = "Hydrology dependencies changed. Regenerate dependent layers.";
            hasError_ = false;
        }

        if (layout_) {
            const auto& generated = layout_->settings;
            const auto current = viewport.layer_revisions();

            if (!viewport.map_layout() ||
                layoutSource_.terrain != current.terrain ||
                layoutSource_.hydrology != current.hydrology ||
                layoutSource_.layout != current.layout ||
                generated.mapSize != settings_.mapSize ||
                generated.poiCount != settings_.poiCount ||
                generated.exitCount != settings_.exitCount ||
                generated.poiSeparationMeters != settings_.poiSeparationMeters ||
                generated.borderClearanceMeters != settings_.borderClearanceMeters ||
                generated.allowedTypes != settings_.allowedTypes ||
                generated.seeds.terrain != settings_.seeds.terrain ||
                generated.seeds.water != settings_.seeds.water ||
                generated.seeds.location != settings_.seeds.location ||
                generated.seeds.road != settings_.seeds.road) {
                clear_layout(viewport);
                status_ = "Map Layout dependencies changed. Regenerate dependent layers.";
                hasError_ = false;
            }
        }

        if (layout_) {
            layout_->settings.seeds.biome = settings_.seeds.biome;
            layout_->settings.seeds.vegetation = settings_.seeds.vegetation;
            layout_->settings.seeds.decoration = settings_.seeds.decoration;
            viewport.update_map_layout_seeds(settings_.seeds);
        }
    }

    void MapLayoutPanel::validate_sources(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& settings,
        viewport::TerrainViewport& viewport
    ) {
        synchronize(viewport, settings);

        if (layer_grid(heightfield) != layer_grid(settings) ||
            layer_grid(heightfield) != viewport.terrain_grid() ||
            world_editor::biome::terrain_source_fingerprint(heightfield) !=
                viewport.terrain_fingerprint()) {
            clear(viewport);
            throw std::runtime_error("Terrain heights changed. Apply Terrain before saving.");
        }

        if (layout_ && layout_->sourceFingerprint != viewport.source_fingerprint()) {
            clear_layout(viewport);
            throw std::runtime_error("Map Layout source changed. Regenerate Map Layout.");
        }
    }

    void MapLayoutPanel::generate_hydrology(
        terrain::Heightfield& heightfield,
        const terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport
    ) {
        validate_sources(heightfield, activeSettings, viewport);
        hydrologySettings_.seed = settings_.seeds.water;

        const terrain::Heightfield& source = baseTerrain_
            ? *baseTerrain_ : heightfield;

        auto generated = world_editor::biome::generate_hydrology(
            source,
            hydrologySettings_
        );

        if (baseTerrain_) {
            viewport.set_terrain(*baseTerrain_);
            heightfield = std::move(*baseTerrain_);
            baseTerrain_.reset();
            correctedTerrainRevision_ = 0;
        }

        clear(viewport);
        hydrology_ = std::move(generated);
        sourceHydrologyTerrain_ = activeSettings;

        viewport.set_hydrology(&*hydrology_);
        hydrologySource_ = viewport.layer_revisions();
        hydrologyTerrainFingerprint_ = viewport.terrain_fingerprint();
        status_ = "Hydrology generated";
        hasError_ = false;
    }

    void MapLayoutPanel::generate_layout(
        terrain::Heightfield& heightfield,
        const terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport
    ) {
        validate_sources(heightfield, activeSettings, viewport);
        settings_.seeds.terrain = activeSettings.seed;

        const world_editor::biome::HydrologyLayer* water =
            hydrology_ ? &*hydrology_ : nullptr;

        if (water) {
            const auto& active = water->settings;

            if (active.seed != settings_.seeds.water ||
                active.riverEnabled != hydrologySettings_.riverEnabled ||
                active.lakeCount != hydrologySettings_.lakeCount ||
                active.riverWidthMeters != hydrologySettings_.riverWidthMeters ||
                active.marshWidthMeters != hydrologySettings_.marshWidthMeters ||
                active.riverShape != hydrologySettings_.riverShape ||
                active.lakeSeparationMeters !=
                    hydrologySettings_.lakeSeparationMeters) {
                throw std::runtime_error(
                    "Hydrology settings changed. Regenerate Hydrology before Map Layout."
                );
            }
        }

        const terrain::Heightfield& source = baseTerrain_
            ? *baseTerrain_ : heightfield;

        auto generated = wg::generate_layout(
            settings_,
            source,
            water
        );

        auto corrected = terrain::correct_for_layout(
            source,
            generated,
            water
        );

        if (!baseTerrain_) {
            baseTerrain_ = heightfield;
        }

        viewport.set_terrain(corrected);
        heightfield = std::move(corrected);
        correctedTerrainRevision_ = viewport.layer_revisions().terrain;

        if (hydrology_) {
            viewport.set_hydrology(&*hydrology_);
            hydrologySource_ = viewport.layer_revisions();
            hydrologyTerrainFingerprint_ = viewport.terrain_fingerprint();
        }

        generated.sourceFingerprint = viewport.source_fingerprint();
        viewport.set_map_layout(&generated);

        layout_ = std::move(generated);
        sourceTerrain_ = activeSettings;
        layoutSource_ = viewport.layer_revisions();
        status_ = "Map Layout generated; Terrain corrected for roads and POIs";
        hasError_ = false;
    }

    void MapLayoutPanel::draw(
        const terrain::Heightfield& heightfield,
        const terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport,
        const WorldStoragePanel& storage
    ) {
        synchronize(viewport, activeSettings);
        ImGui::Begin("Map Layout");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "MAP LAYOUT SETTINGS"
        );

        ImGui::TextDisabled(
            "Procedural planning / Version 1"
        );

        ImGui::Separator();

        if (ImGui::CollapsingHeader(
            "01  Map Configuration",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            constexpr const char* sizes[] = {
                "512 x 512 m",
                "1024 x 1024 m",
                "2048 x 2048 m",
                "4096 x 4096 m"
            };

            int selected = selected_map_size(settings_.mapSize);

            ImGui::SetNextItemWidth(-1.0f);

            if (ImGui::Combo(
                "##MapSize",
                &selected,
                sizes,
                IM_ARRAYSIZE(sizes)
            )) {
                constexpr wg::MapSize mapSizes[] = {
                    wg::MapSize::Small,
                    wg::MapSize::Medium,
                    wg::MapSize::Large,
                    wg::MapSize::ExtraLarge
                };

                settings_.mapSize = mapSizes[selected];
            }

            ImGui::TextDisabled("Map Size");

            ImGui::InputScalar(
                "POI Count",
                ImGuiDataType_U32,
                &settings_.poiCount
            );

            ImGui::InputScalar(
                "Exit Count",
                ImGuiDataType_U32,
                &settings_.exitCount
            );

            ImGui::DragFloat(
                "POI Separation",
                &settings_.poiSeparationMeters,
                1.0f,
                0.0f,
                512.0f,
                "%.0f m"
            );

            ImGui::DragFloat(
                "Border Clearance",
                &settings_.borderClearanceMeters,
                1.0f,
                0.0f,
                512.0f,
                "%.0f m"
            );
        }

        if (ImGui::CollapsingHeader(
            "02  Location Catalog",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            constexpr const char* presets[] = {
                "Automatic / All Suitable",
                "Rural / Countryside",
                "Small POI",
                "Urban / Industrial"
            };

            ImGui::SetNextItemWidth(-1.0f);

            if (ImGui::Combo(
                "##POIPreset",
                &poiPreset_,
                presets,
                IM_ARRAYSIZE(presets)
            )) {
                apply_preset(settings_, poiPreset_);
            }
        }

        if (ImGui::CollapsingHeader("03  Generation Seeds")) {
            auto& seeds = settings_.seeds;

            ImGui::InputScalar(
                "Terrain",
                ImGuiDataType_U32,
                &seeds.terrain
            );

            ImGui::InputScalar(
                "Water",
                ImGuiDataType_U32,
                &seeds.water
            );

            ImGui::InputScalar(
                "Biome",
                ImGuiDataType_U32,
                &seeds.biome
            );

            ImGui::InputScalar(
                "Vegetation",
                ImGuiDataType_U32,
                &seeds.vegetation
            );

            ImGui::InputScalar(
                "Location",
                ImGuiDataType_U32,
                &seeds.location
            );

            ImGui::InputScalar(
                "Road",
                ImGuiDataType_U32,
                &seeds.road
            );

            ImGui::InputScalar(
                "Decoration",
                ImGuiDataType_U32,
                &seeds.decoration
            );
        }

        synchronize(viewport, activeSettings);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const auto mapMeters = wg::map_size_meters(
            settings_.mapSize
        );

        const std::uint32_t samples = mapMeters / 8 + 1;

        const bool matchingTerrain =
            heightfield.valid() &&
            heightfield.samplesX == samples &&
            heightfield.samplesZ == samples &&
            heightfield.spacingMeters == 8.0f &&
            activeSettings.seed == settings_.seeds.terrain;

        if (!matchingTerrain) {
            ImGui::TextWrapped(
                "The active Terrain does not match "
                "the selected Map size or TerrainSeed. "
                "Select Terrain in World Generator."
            );
        }

        ImGui::TextDisabled(
            "Use World Generator to generate selected layers."
        );

        if (ImGui::CollapsingHeader(
            "04  Hydrology",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            ImGui::TextDisabled(
                "Water masks / River / Lakes / Marshes"
            );

            ImGui::Checkbox(
                "Generate River",
                &hydrologySettings_.riverEnabled
            );

            constexpr const char* riverShapes[] = {
                "Auto",
                "Straight",
                "L Shape",
                "U Shape",
                "X Shape"
            };

            int selectedShape = static_cast<int>(
                hydrologySettings_.riverShape
            );

            ImGui::SetNextItemWidth(-1.0f);

            if (ImGui::Combo(
                "River Shape",
                &selectedShape,
                riverShapes,
                IM_ARRAYSIZE(riverShapes)
            )) {
                hydrologySettings_.riverShape =
                    static_cast<world_editor::biome::RiverShape>(
                        selectedShape
                    );
            }

            ImGui::SetNextItemWidth(160.0f);

            int lakeCount = static_cast<int>(
                hydrologySettings_.lakeCount
            );

            if (ImGui::SliderInt(
                "Lake Count",
                &lakeCount,
                0,
                8
            )) {
                hydrologySettings_.lakeCount =
                    static_cast<std::uint32_t>(lakeCount);
            }

            ImGui::SliderFloat(
                "River Width",
                &hydrologySettings_.riverWidthMeters,
                16.0f,
                96.0f,
                "%.0f m"
            );

            ImGui::SliderFloat(
                "Marsh Width",
                &hydrologySettings_.marshWidthMeters,
                0.0f,
                96.0f,
                "%.0f m"
            );

            ImGui::SliderFloat(
                "Lake Separation",
                &hydrologySettings_.lakeSeparationMeters,
                100.0f,
                800.0f,
                "%.0f m"
            );

            synchronize(viewport, activeSettings);

            if (hydrology_) {
                ImGui::Text(
                    "River points: %u",
                    static_cast<unsigned>(
                        hydrology_->river.size()
                    )
                );

                ImGui::Text(
                    "Lakes: %u",
                    static_cast<unsigned>(
                        hydrology_->lakes.size()
                    )
                );

                ImGui::Text(
                    "Marsh markers: %u",
                    static_cast<unsigned>(
                        hydrology_->marshMarkers.size()
                    )
                );
            } else {
                ImGui::TextDisabled(
                    "No water mask generated yet"
                );
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled("05 Layout Storage");

        ImGui::BeginDisabled(!layout_.has_value() || viewport.is_streaming());

        if (ImGui::Button("Save Layout")) {
            try {
                const auto directory = storage.world_directory();
                const auto savedWorld = terrain::load_world(directory);

                if (!same_terrain(savedWorld.settings, activeSettings) ||
                    world_editor::biome::terrain_source_fingerprint(
                        savedWorld.terrain
                    ) != viewport.terrain_fingerprint()) {
                    throw std::runtime_error(
                        "Save matching Terrain World first"
                    );
                }

                save_hydrology_for_world(
                    heightfield,
                    activeSettings,
                    directory
                );
                save_layout_for_world(
                    heightfield,
                    activeSettings,
                    viewport,
                    directory
                );

                status_ = "Map Layout and Hydrology saved";
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::Button("Load Layout")) {
            try {
                const auto directory = storage.world_directory();
                const auto savedTerrain =
                    terrain::read_world_settings(directory);

                if (!same_terrain(savedTerrain, activeSettings)) {
                    throw std::runtime_error(
                        "Load or stream the selected World first"
                    );
                }

                if (!restore_layout_from_world(
                    heightfield,
                    savedTerrain,
                    viewport,
                    directory
                )) {
                    throw std::runtime_error("No saved Map Layout");
                }

                status_ = "Map Layout loaded";
                hasError_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
            }
        }

        if (!status_.empty()) {
            ImGui::Spacing();

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

    void MapLayoutPanel::draw_inspector() const {
        ImGui::Begin("Map Inspector");

        ImGui::TextUnformatted("MAP INSPECTOR");
        ImGui::Separator();

        if (!layout_) {
            ImGui::TextDisabled("No active Map Layout");

            ImGui::TextWrapped(
                "Generate or load a Map Layout "
                "to inspect POI and road connectivity."
            );

            ImGui::End();
            return;
        }

        const auto& layout = *layout_;

        ImGui::Text(
            "Map: %u x %u m",
            wg::map_size_meters(layout.settings.mapSize),
            wg::map_size_meters(layout.settings.mapSize)
        );

        ImGui::Text(
            "POI: %u  |  Exits: %u",
            static_cast<unsigned>(layout.pois.size()),
            static_cast<unsigned>(layout.exits.size())
        );

        ImGui::Text(
            "Road edges: %u",
            static_cast<unsigned>(layout.roads.edges.size())
        );

        const bool connected = world_editor::road::is_connected(
            layout.roads
        );

        ImGui::TextColored(
            connected
                ? ImVec4(0.38f, 0.85f, 0.74f, 1.0f)
                : ImVec4(1.0f, 0.44f, 0.40f, 1.0f),
            connected ? "GRAPH CONNECTED" : "GRAPH DISCONNECTED"
        );

        ImGui::Separator();

        if (ImGui::CollapsingHeader(
            "Locations",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            for (const auto& poi : layout.pois) {
                ImGui::PushID(static_cast<int>(poi.id));

                const bool open = ImGui::TreeNode(
                    "POI",
                    "POI %u - %s",
                    poi.id + 1,
                    poi_name(poi.type)
                );

                if (open) {
                    ImGui::Text(
                        "Position: %.0f, %.0f",
                        poi.centerX,
                        poi.centerZ
                    );

                    ImGui::Text(
                        "Footprint: %.0f x %.0f m",
                        poi.widthMeters,
                        poi.depthMeters
                    );

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader(
            "Map Exits",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            constexpr const char* sides[] = {
                "North",
                "East",
                "South",
                "West"
            };

            for (const auto& exit : layout.exits) {
                const auto side = static_cast<unsigned>(exit.side);

                ImGui::BulletText(
                    "Exit %u - %s (%.0f, %.0f)",
                    exit.id + 1,
                    side < 4 ? sides[side] : "Unknown",
                    exit.x,
                    exit.z
                );
            }
        }

        ImGui::End();
    }

}
