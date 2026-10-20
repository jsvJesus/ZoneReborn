#include "Editor/WorldGenerator/WorldGeneratorPanel.h"

#include "Editor/UI/OutputLog.h"
#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/WorldGenerator/BiomeMaterialsPanel.h"
#include "Editor/WorldGenerator/MapLayoutPanel.h"
#include "Editor/WorldGenerator/TerrainReliefPanel.h"

#include "WorldEditor/WorldGenerator/MapGenerationSettings.h"

#include "imgui.h"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <utility>

namespace editor::world_generator {

    namespace {

        namespace terrain = world_editor::terrain;
        namespace wg = world_editor::world_generator;

        [[nodiscard]] int map_size_index(wg::MapSize size) noexcept {
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

    }

    void WorldGeneratorPanel::draw(
        terrain::Heightfield& heightfield,
        terrain::Settings& settings,
        terrain::Settings& activeSettings,
        viewport::TerrainViewport& viewport,
        MapLayoutPanel& mapPanel,
        BiomeMaterialsPanel& biomePanel,
        TerrainReliefPanel& reliefPanel,
        ui::OutputLog& outputLog
    ) {
        if (!ImGui::Begin("World Generator")) {
            ImGui::End();
            return;
        }

        auto& map = mapPanel.generation_settings();
        auto& water = mapPanel.hydrology_settings();
        auto& biome = biomePanel.generation_settings();

        ImGui::TextUnformatted("GENERATE MAP");
        ImGui::TextDisabled("Choose layers, then generate in dependency order.");
        ImGui::Separator();

        ImGui::SeparatorText("Layers to Generate");
        ImGui::Checkbox("Terrain##GenerateLayer", &terrainEnabled_);
        ImGui::Checkbox("Hydrology##GenerateLayer", &hydrologyEnabled_);
        ImGui::Checkbox("Roads, POI and Exits##GenerateLayer", &layoutEnabled_);
        ImGui::Checkbox("Biomes and Materials##GenerateLayer", &biomesEnabled_);

        const bool hasSelection = terrainEnabled_ || hydrologyEnabled_ ||
            layoutEnabled_ || biomesEnabled_;

        if (terrainEnabled_) {
            ImGui::TextWrapped(
                "Terrain resets existing water, roads, biomes and vegetation. "
                "Select dependent layers to rebuild them in the same run."
            );
        } else if (hydrologyEnabled_) {
            ImGui::TextWrapped(
                "Hydrology resets existing roads, biomes and vegetation. "
                "Select dependent layers to rebuild them in the same run."
            );
        } else if (layoutEnabled_) {
            ImGui::TextWrapped(
                "Road and POI changes reset existing biomes and vegetation. "
                "Select Biomes to rebuild them in the same run."
            );
        } else if (biomesEnabled_) {
            ImGui::TextWrapped(
                "Biome changes reset existing vegetation."
            );
        } else {
            ImGui::TextDisabled("Select at least one layer.");
        }

        ImGui::SeparatorText("Generation Settings");

        constexpr const char* sizes[] = {
            "512 x 512 m",
            "1024 x 1024 m",
            "2048 x 2048 m",
            "4096 x 4096 m"
        };
        constexpr wg::MapSize mapSizes[] = {
            wg::MapSize::Small,
            wg::MapSize::Medium,
            wg::MapSize::Large,
            wg::MapSize::ExtraLarge
        };

        int selectedSize = map_size_index(map.mapSize);

        if (ImGui::Combo(
            "Map Size",
            &selectedSize,
            sizes,
            IM_ARRAYSIZE(sizes)
        )) {
            map.mapSize = mapSizes[selectedSize];
        }

        if (ImGui::CollapsingHeader(
            "Terrain",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            ImGui::InputScalar(
                "Terrain Seed",
                ImGuiDataType_U32,
                &map.seeds.terrain
            );
            ImGui::SliderFloat(
                "Relief",
                &settings.reliefMeters,
                2.0f,
                50.0f,
                "%.1f m"
            );
            reliefPanel.draw();
        }

        if (ImGui::CollapsingHeader("Hydrology")) {
            ImGui::InputScalar(
                "Water Seed",
                ImGuiDataType_U32,
                &map.seeds.water
            );
            ImGui::Checkbox("River", &water.riverEnabled);
            ImGui::SliderFloat(
                "River Width",
                &water.riverWidthMeters,
                16.0f,
                96.0f,
                "%.0f m"
            );

            int lakeCount = static_cast<int>(water.lakeCount);

            if (ImGui::SliderInt("Lake Count", &lakeCount, 0, 8)) {
                water.lakeCount = static_cast<std::uint32_t>(lakeCount);
            }
        }

        if (ImGui::CollapsingHeader("Roads, POI and Exits")) {
            ImGui::InputScalar(
                "Location Seed",
                ImGuiDataType_U32,
                &map.seeds.location
            );
            ImGui::InputScalar(
                "Road Seed",
                ImGuiDataType_U32,
                &map.seeds.road
            );
            ImGui::InputScalar(
                "POI Count",
                ImGuiDataType_U32,
                &map.poiCount
            );
            ImGui::InputScalar(
                "Exit Count",
                ImGuiDataType_U32,
                &map.exitCount
            );
        }

        if (ImGui::CollapsingHeader("Biomes and Materials")) {
            ImGui::InputScalar(
                "Biome Seed",
                ImGuiDataType_U32,
                &map.seeds.biome
            );
            ImGui::SliderFloat(
                "Moisture",
                &biome.moisture,
                0.0f,
                1.0f,
                "%.2f"
            );
            ImGui::SliderFloat(
                "Forest Coverage",
                &biome.forestCoverage,
                0.0f,
                1.0f,
                "%.2f"
            );
        }

        ImGui::Separator();
        ImGui::BeginDisabled(viewport.is_streaming() || !hasSelection);

        if (ImGui::Button("Generate Map", ImVec2(-1.0f, 42.0f))) {
            try {
                const auto mapMeters = wg::map_size_meters(map.mapSize);
                const auto samples = mapMeters / 8 + 1;

                if (terrainEnabled_) {
                    terrain::Settings next = settings;
                    next.samplesX = samples;
                    next.samplesZ = samples;
                    next.spacingMeters = 8.0f;
                    next.seed = map.seeds.terrain;

                    auto generated = reliefPanel.generate(next);
                    viewport.set_terrain(generated);
                    heightfield = std::move(generated);
                    settings = next;
                    activeSettings = next;
                    mapPanel.clear(viewport);
                    biomePanel.clear(viewport);
                    outputLog.add("Terrain generated");
                } else if (!heightfield.valid() ||
                    heightfield.samplesX != samples ||
                    heightfield.samplesZ != samples ||
                    heightfield.spacingMeters != 8.0f ||
                    activeSettings.seed != map.seeds.terrain) {
                    throw std::runtime_error(
                        "Active Terrain does not match map size or Terrain Seed"
                    );
                }

                if (hydrologyEnabled_) {
                    mapPanel.generate_hydrology(
                        heightfield,
                        activeSettings,
                        viewport
                    );
                    outputLog.add("Hydrology generated");
                }

                if (layoutEnabled_) {
                    mapPanel.generate_layout(
                        heightfield,
                        activeSettings,
                        viewport
                    );
                    outputLog.add("Roads and POI generated");
                }

                if (biomesEnabled_) {
                    biomePanel.generate(
                        heightfield,
                        activeSettings,
                        viewport,
                        map.seeds
                    );
                    outputLog.add("Biomes generated");
                }

                status_ = "Selected layers generated";
                hasError_ = false;
                terrainEnabled_ = false;
                hydrologyEnabled_ = false;
                layoutEnabled_ = false;
                biomesEnabled_ = false;
            } catch (const std::exception& error) {
                status_ = error.what();
                hasError_ = true;
                outputLog.add(status_, ui::LogLevel::Error);
            }
        }

        ImGui::EndDisabled();

        if (!status_.empty()) {
            ImGui::TextColored(
                hasError_
                    ? ImVec4(1.0f, 0.48f, 0.42f, 1.0f)
                    : ImVec4(0.35f, 0.67f, 0.98f, 1.0f),
                "%s",
                status_.c_str()
            );
        }

        ImGui::End();
    }

}
