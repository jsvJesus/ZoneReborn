#include "Editor/UI/WorldInfoPanel.h"

#include "Editor/Viewport/TerrainViewport.h"

#include "imgui.h"

namespace editor::ui {

    void draw_world_info(const viewport::TerrainViewport& viewport) {
        if (!ImGui::Begin("World Info")) {
            ImGui::End();
            return;
        }

        const auto& grid = viewport.terrain_grid();
        const auto& revisions = viewport.layer_revisions();

        ImGui::Text("Terrain grid: %u x %u", grid.samplesX, grid.samplesZ);
        ImGui::Text("Terrain revision: %llu", revisions.terrain);
        ImGui::Text("Hydrology revision: %llu", revisions.hydrology);
        ImGui::Text("Layout revision: %llu", revisions.layout);
        ImGui::Separator();
        ImGui::Text("Water: %s", viewport.hydrology() ? "Ready" : "Empty");
        ImGui::Text("Biomes: %s", viewport.has_biomes() ? "Ready" : "Empty");
        ImGui::Text("Vegetation: %s", viewport.has_vegetation() ? "Ready" : "Empty");
        ImGui::Text("Streaming: %s", viewport.is_streaming() ? "On" : "Off");

        if (const auto* layout = viewport.map_layout()) {
            ImGui::Separator();
            ImGui::Text("POI: %zu", layout->pois.size());
            ImGui::Text("Exits: %zu", layout->exits.size());
            ImGui::Text("Roads: %zu", layout->roads.edges.size());
        }

        ImGui::End();
    }

}
