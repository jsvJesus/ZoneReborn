#include "Editor/UI/WorldOutlinerPanel.h"

#include "Editor/UI/MapSelection.h"
#include "Editor/Viewport/TerrainViewport.h"

#include "WorldEditor/Settlement/POIPlacement.h"
#include "WorldEditor/Road/RoadGraph.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace editor::ui {

    namespace {

        [[nodiscard]] const char* poi_name(
            const world_editor::settlement::POIType type
        ) noexcept {
            using Type = world_editor::settlement::POIType;

            switch (type) {
            case Type::Checkpoint:
                return "Checkpoint";
            case Type::Farm:
                return "Farm";
            case Type::Village:
                return "Village";
            case Type::Industrial:
                return "Industrial";
            case Type::Town:
                return "Town";
            }

            return "Unknown";
        }

        [[nodiscard]] bool contains_text(
            const char* text,
            const char* query
        ) {
            if (query[0] == '\0') {
                return true;
            }

            const char* result = std::search(
                text,
                text + std::strlen(text),
                query,
                query + std::strlen(query),
                [](const char a, const char b) {
                    return std::tolower(
                        static_cast<unsigned char>(a)
                    ) == std::tolower(
                        static_cast<unsigned char>(b)
                    );
                }
            );

            return result != text + std::strlen(text);
        }

    }

    void WorldOutlinerPanel::draw(
        viewport::TerrainViewport& viewport
    ) {
        if (!ImGui::Begin("Outliner")) {
            ImGui::End();
            return;
        }

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "WORLD OUTLINER"
        );

        ImGui::TextDisabled("Scene hierarchy");

        ImGui::Separator();

        ImGui::SetNextItemWidth(-1.0f);

        ImGui::InputTextWithHint(
            "##OutlinerSearch",
            "Search locations, exits, roads...",
            search_,
            sizeof(search_)
        );

        ImGui::Spacing();

        const auto* layout = viewport.map_layout();

        if (!layout) {
            ImGui::TextDisabled("No active Map Layout");
            ImGui::End();
            return;
        }

        const MapSelection selection = viewport.selection();

        ImGui::BeginDisabled(!selection.valid());

        if (ImGui::SmallButton("Focus Selected")) {
            viewport.focus_selection();
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Clear Selection")) {
            viewport.select({});
        }

        ImGui::EndDisabled();

        ImGui::TextDisabled(
            "Map: %u x %u m",
            world_editor::world_generator::map_size_meters(
                layout->settings.mapSize
            ),
            world_editor::world_generator::map_size_meters(
                layout->settings.mapSize
            )
        );

        ImGui::Spacing();

        if (ImGui::CollapsingHeader(
            "Locations",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            for (const auto& poi : layout->pois) {
                char label[128]{};

                std::snprintf(
                    label,
                    sizeof(label),
                    "POI %u  |  %s",
                    poi.id + 1,
                    poi_name(poi.type)
                );

                if (!contains_text(label, search_)) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(poi.id));

                const bool selected =
                    selection.kind == MapSelectionKind::POI &&
                    selection.id == poi.id;

                if (ImGui::Selectable(
                    label,
                    selected,
                    ImGuiSelectableFlags_AllowDoubleClick
                )) {
                    viewport.select({
                        MapSelectionKind::POI,
                        poi.id
                    });

                    if (ImGui::IsMouseDoubleClicked(
                        ImGuiMouseButton_Left
                    )) {
                        viewport.focus_selection();
                    }
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

            for (const auto& exit : layout->exits) {
                char label[128]{};

                const auto side = static_cast<unsigned>(
                    exit.side
                );

                std::snprintf(
                    label,
                    sizeof(label),
                    "Exit %u  |  %s",
                    exit.id + 1,
                    side < 4 ? sides[side] : "Unknown"
                );

                if (!contains_text(label, search_)) {
                    continue;
                }

                ImGui::PushID(
                    static_cast<int>(1000u + exit.id)
                );

                const bool selected =
                    selection.kind == MapSelectionKind::Exit &&
                    selection.id == exit.id;

                if (ImGui::Selectable(
                    label,
                    selected,
                    ImGuiSelectableFlags_AllowDoubleClick
                )) {
                    viewport.select({
                        MapSelectionKind::Exit,
                        exit.id
                    });

                    if (ImGui::IsMouseDoubleClicked(
                        ImGuiMouseButton_Left
                    )) {
                        viewport.focus_selection();
                    }
                }

                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Road Network")) {
            ImGui::Text(
                "Nodes: %u",
                static_cast<unsigned>(layout->roads.nodes.size())
            );

            ImGui::Text(
                "Edges: %u",
                static_cast<unsigned>(layout->roads.edges.size())
            );

            ImGui::PushID("RoadEdges");

            for (std::size_t index = 0;
                 index < layout->roads.edges.size();
                 ++index) {
                const auto& edge = layout->roads.edges[index];
                char label[160]{};

                if (edge.from < layout->roads.nodes.size() &&
                    edge.to < layout->roads.nodes.size()) {
                    const auto& from = layout->roads.nodes[edge.from];
                    const auto& to = layout->roads.nodes[edge.to];

                    std::snprintf(label, sizeof(label),
                        "Road %zu  |  %s %u - %s %u",
                        index + 1,
                        from.kind == world_editor::road::RoadNodeKind::POI
                            ? "POI" : "Exit",
                        from.referenceId + 1,
                        to.kind == world_editor::road::RoadNodeKind::POI
                            ? "POI" : "Exit",
                        to.referenceId + 1);
                } else {
                    std::snprintf(label, sizeof(label), "Road %zu", index + 1);
                }

                if (!contains_text(label, search_)) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(index));

                if (ImGui::Selectable(label,
                    selection.kind == MapSelectionKind::Road &&
                    selection.id == index,
                    ImGuiSelectableFlags_AllowDoubleClick)) {
                    viewport.select({
                        MapSelectionKind::Road,
                        static_cast<std::uint32_t>(index)
                    });

                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        viewport.focus_selection();
                    }
                }

                ImGui::PopID();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }

}
