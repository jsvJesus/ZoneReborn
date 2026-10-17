#include "Editor/UI/MapInspectorPanel.h"

#include "Editor/UI/MapSelection.h"
#include "Editor/Viewport/TerrainViewport.h"

#include "WorldEditor/Settlement/POIPlacement.h"
#include "WorldEditor/Road/RoadGraph.h"

#include "imgui.h"

#include <algorithm>
#include <cstdint>

namespace editor::ui {

    namespace {

        using namespace world_editor;

        [[nodiscard]] const char* poi_name(
            const settlement::POIType type
        ) noexcept {
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

        void draw_connections(
            const world_generator::MapLayout& layout,
            const MapSelection selection
        ) {
            const road::RoadNodeKind targetKind =
                selection.kind == MapSelectionKind::POI
                    ? road::RoadNodeKind::POI
                    : road::RoadNodeKind::Exit;

            std::size_t selectedIndex = layout.roads.nodes.size();

            for (std::size_t i = 0;
                 i < layout.roads.nodes.size();
                 ++i) {
                const auto& node = layout.roads.nodes[i];

                if (node.kind == targetKind &&
                    node.referenceId == selection.id) {
                    selectedIndex = i;
                    break;
                }
            }

            if (selectedIndex == layout.roads.nodes.size()) {
                ImGui::TextDisabled("Road node not found");
                return;
            }

            std::uint32_t connections = 0;

            for (const auto& edge : layout.roads.edges) {
                std::uint32_t neighbor = 0;

                if (edge.from == selectedIndex) {
                    neighbor = edge.to;
                } else if (edge.to == selectedIndex) {
                    neighbor = edge.from;
                } else {
                    continue;
                }

                if (neighbor >= layout.roads.nodes.size()) {
                    continue;
                }

                ++connections;

                const auto& node = layout.roads.nodes[neighbor];

                ImGui::BulletText(
                    "%s %u",
                    node.kind == road::RoadNodeKind::POI
                        ? "POI"
                        : "Exit",
                    node.referenceId + 1
                );
            }

            if (connections == 0) {
                ImGui::TextDisabled("No road connections");
            }
        }

    }

    void MapInspectorPanel::draw(
        const viewport::TerrainViewport& viewport
    ) const {
        if (!ImGui::Begin("Map Inspector")) {
            ImGui::End();
            return;
        }

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "INSPECTOR"
        );

        ImGui::TextDisabled("Object properties");

        ImGui::Separator();

        const auto* layout = viewport.map_layout();

        if (!layout) {
            ImGui::TextDisabled("No active Map Layout");
            ImGui::End();
            return;
        }

        const MapSelection selection = viewport.selection();

        if (!selection.valid()) {
            const auto side =
                world_generator::map_size_meters(
                    layout->settings.mapSize
                );

            ImGui::TextUnformatted("MAP OVERVIEW");
            ImGui::Separator();

            ImGui::Text("Size: %u x %u m", side, side);

            ImGui::Text(
                "Locations: %u",
                static_cast<unsigned>(layout->pois.size())
            );

            ImGui::Text(
                "Exits: %u",
                static_cast<unsigned>(layout->exits.size())
            );

            ImGui::Text(
                "Road edges: %u",
                static_cast<unsigned>(
                    layout->roads.edges.size()
                )
            );

            const bool connected = road::is_connected(
                layout->roads
            );

            ImGui::TextColored(
                connected
                    ? ImVec4(0.38f, 0.85f, 0.74f, 1.0f)
                    : ImVec4(1.0f, 0.40f, 0.40f, 1.0f),
                connected
                    ? "ROAD GRAPH CONNECTED"
                    : "ROAD GRAPH DISCONNECTED"
            );

            ImGui::Spacing();

            ImGui::TextWrapped(
                "Select a location or an exit in "
                "World Outliner or Terrain Viewport."
            );

            ImGui::End();
            return;
        }

        if (selection.kind == MapSelectionKind::POI) {
            const auto it = std::find_if(
                layout->pois.begin(),
                layout->pois.end(),
                [selection](const auto& poi) {
                    return poi.id == selection.id;
                }
            );

            if (it == layout->pois.end()) {
                ImGui::TextDisabled("Selected POI not found");
                ImGui::End();
                return;
            }

            const auto spec = settlement::specification(
                it->type
            );

            ImGui::Text(
                "POI %u",
                it->id + 1
            );

            ImGui::TextColored(
                ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
                "%s",
                poi_name(it->type)
            );

            ImGui::SeparatorText("Transform");

            ImGui::Text(
                "X: %.2f m",
                it->centerX
            );

            ImGui::Text(
                "Y: %.2f m",
                it->centerY
            );

            ImGui::Text(
                "Z: %.2f m",
                it->centerZ
            );

            ImGui::SeparatorText("Footprint");

            ImGui::Text(
                "Width: %.0f m",
                it->widthMeters
            );

            ImGui::Text(
                "Depth: %.0f m",
                it->depthMeters
            );

            ImGui::Text(
                "Allowed slope: %.1f deg",
                spec.maxSlopeDegrees
            );

            ImGui::TextDisabled(
                "Slope value shown is a placement limit."
            );
        } else if (selection.kind == MapSelectionKind::Exit) {
            const auto it = std::find_if(
                layout->exits.begin(),
                layout->exits.end(),
                [selection](const auto& exit) {
                    return exit.id == selection.id;
                }
            );

            if (it == layout->exits.end()) {
                ImGui::TextDisabled("Selected exit not found");
                ImGui::End();
                return;
            }

            constexpr const char* sides[] = {
                "North",
                "East",
                "South",
                "West"
            };

            const auto side = static_cast<unsigned>(
                it->side
            );

            ImGui::Text("Exit %u", it->id + 1);

            ImGui::SeparatorText("Transition");

            ImGui::Text(
                "Boundary: %s",
                side < 4 ? sides[side] : "Unknown"
            );

            ImGui::Text(
                "Position: %.0f, %.0f",
                it->x,
                it->z
            );

            ImGui::Text(
                "Elevation: %.2f m",
                it->y
            );
        }

        ImGui::SeparatorText("Road Connections");

        draw_connections(*layout, selection);

        ImGui::Spacing();

        ImGui::TextDisabled(
            "Read-only planning mode"
        );

        ImGui::End();
    }

}
