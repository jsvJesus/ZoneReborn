#include "Editor/Viewport/ViewportToolbar.h"

#include "Editor/Viewport/TerrainWorldStreamer.h"

#include <imgui_internal.h>
#include "imgui.h"

namespace editor::viewport {

    namespace {

        void draw_layers(ViewportToolbarState& state) {
            if (!ImGui::BeginPopup("ViewportLayersPopup")) {
                return;
            }

            ImGui::SeparatorText("Map Layout");

            ImGui::Checkbox(
                "Show Layout",
                &state.showLayout
            );

            ImGui::Separator();

            ImGui::BeginDisabled(!state.showLayout);

            ImGui::Checkbox(
                "POI Footprints",
                &state.layers.poi
            );

            ImGui::Checkbox(
                "Road Graph",
                &state.layers.roads
            );

            ImGui::Checkbox(
                "Map Exits",
                &state.layers.exits
            );

            ImGui::Checkbox(
                "Object Labels",
                &state.layers.labels
            );

            ImGui::EndDisabled();

            ImGui::SeparatorText("Hydrology");

            ImGui::Checkbox(
                "Water / Marshes",
                &state.layers.water
            );

            ImGui::EndPopup();
        }

        void draw_display(
            ViewportToolbarState& state,
            TerrainWorldStreamer* streamer
        ) {
            if (!ImGui::BeginPopup("ViewportDisplayPopup")) {
                return;
            }

            ImGui::SeparatorText("Rendering");

            ImGui::Checkbox(
                "Wireframe",
                &state.wireframe
            );

            ImGui::Checkbox(
                "Frustum Culling",
                &state.frustumCulling
            );

            ImGui::SeparatorText("Level of Detail");

            ImGui::Checkbox(
                "Automatic LOD",
                &state.automaticLod
            );

            ImGui::SetNextItemWidth(190.0f);

            if (state.automaticLod) {
                ImGui::SliderFloat(
                    "LOD Distance",
                    &state.lodDistance,
                    0.5f,
                    3.0f,
                    "%.2f"
                );
            } else {
                ImGui::SliderInt(
                    "Fixed LOD",
                    &state.fixedLod,
                    0,
                    3
                );
            }

            if (streamer) {
                ImGui::SeparatorText("Terrain Streaming");

                int radius = streamer->radius();

                ImGui::SetNextItemWidth(190.0f);

                if (ImGui::SliderInt(
                    "Stream Radius",
                    &radius,
                    1,
                    4
                )) {
                    streamer->set_radius(radius);
                }
            }

            ImGui::EndPopup();
        }

        void draw_camera(ViewportToolbarState& state) {
            if (!ImGui::BeginPopup("ViewportCameraPopup")) {
                return;
            }

            ImGui::SeparatorText("Camera Sensitivity");

            ImGui::SetNextItemWidth(180.0f);

            ImGui::SliderFloat(
                "Orbit",
                &state.orbitSpeed,
                0.2f,
                3.0f,
                "%.2f"
            );

            ImGui::SetNextItemWidth(180.0f);

            ImGui::SliderFloat(
                "Pan",
                &state.panSpeed,
                0.2f,
                3.0f,
                "%.2f"
            );

            ImGui::SetNextItemWidth(180.0f);

            ImGui::SliderFloat(
                "Zoom",
                &state.zoomSpeed,
                0.2f,
                3.0f,
                "%.2f"
            );

            ImGui::Separator();

            ImGui::TextDisabled("RMB: Orbit");
            ImGui::TextDisabled("MMB: Pan");
            ImGui::TextDisabled("Wheel: Zoom");
            ImGui::TextDisabled("WASD: Move");
            ImGui::TextDisabled("F: Focus selection");
            ImGui::TextDisabled("Home: Reset camera");
            ImGui::TextDisabled("Shift: Fast movement");
            ImGui::TextDisabled("Ctrl: Precise movement");

            ImGui::EndPopup();
        }

    }

    ViewportToolbarActions draw_viewport_toolbar(
        ViewportToolbarState& state,
        const bool hasSelection,
        TerrainWorldStreamer* streamer
    ) {
        ViewportToolbarActions actions;

        if (ImGui::Button("Home")) {
            actions.resetCamera = true;
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(!hasSelection);

        if (ImGui::Button("Focus")) {
            actions.focusSelection = true;
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Top")) {
            actions.topView = true;
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        if (ImGui::Button("Layers")) {
            ImGui::OpenPopup("ViewportLayersPopup");
        }

        ImGui::SameLine();

        if (ImGui::Button("Display")) {
            ImGui::OpenPopup("ViewportDisplayPopup");
        }

        ImGui::SameLine();

        if (ImGui::Button("Camera")) {
            ImGui::OpenPopup("ViewportCameraPopup");
        }

        draw_layers(state);
        draw_display(state, streamer);
        draw_camera(state);

        return actions;
    }

}
