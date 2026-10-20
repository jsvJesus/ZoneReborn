#include "Editor/Viewport/ViewportToolbar.h"

#include "Editor/Viewport/TerrainWorldStreamer.h"

#include "imgui.h"

namespace editor::viewport {

    namespace {

        void draw_layers(ViewportToolbarState& state) {
            if (!ImGui::BeginPopup("ViewportLayersPopup")) {
                return;
            }

            ImGui::SeparatorText("Surfaces");
            ImGui::Checkbox("Road Surface", &state.roadSurface);
            ImGui::Checkbox("Water Surface", &state.layers.water);

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
                "Hydrology Guides",
                &state.waterGuides
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

            if (ImGui::MenuItem("Lit", nullptr, !state.wireframe)) {
                state.wireframe = false;
            }

            if (ImGui::MenuItem("Wireframe", nullptr, state.wireframe)) {
                state.wireframe = true;
            }

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

            if (ImGui::MenuItem("Perspective", nullptr,
                state.cameraView == CameraView::Perspective)) {
                state.cameraView = CameraView::Perspective;
            }

            if (ImGui::MenuItem("Top", nullptr,
                state.cameraView == CameraView::Top)) {
                state.cameraView = CameraView::Top;
            }

            if (ImGui::MenuItem("Front", nullptr,
                state.cameraView == CameraView::Front)) {
                state.cameraView = CameraView::Front;
            }

            if (ImGui::MenuItem("Right", nullptr,
                state.cameraView == CameraView::Right)) {
                state.cameraView = CameraView::Right;
            }

            ImGui::Separator();
            ImGui::BeginDisabled(state.cameraView != CameraView::Perspective);
            ImGui::SetNextItemWidth(180.0f);
            ImGui::SliderFloat(
                "Field of View",
                &state.fieldOfViewDegrees,
                30.0f,
                100.0f,
                "%.0f deg"
            );
            ImGui::EndDisabled();

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

            ImGui::TextDisabled("RMB: Orbit / Pan in orthographic views");
            ImGui::TextDisabled("MMB: Pan");
            ImGui::TextDisabled("Wheel: Zoom");
            ImGui::TextDisabled("WASD: Move");
            ImGui::TextDisabled("F: Focus selection");
            ImGui::TextDisabled("Home: Reset camera");
            ImGui::TextDisabled("Esc: Clear selection");
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

        if (ImGui::Button(state.wireframe ? "Wireframe" : "Lit")) {
            ImGui::OpenPopup("ViewportDisplayPopup");
        }

        ImGui::SameLine();

        if (ImGui::Button("Show")) {
            ImGui::OpenPopup("ViewportLayersPopup");
        }

        ImGui::SameLine();

        const char* viewName = state.cameraView == CameraView::Top
            ? "Top"
            : state.cameraView == CameraView::Front
                ? "Front"
                : state.cameraView == CameraView::Right
                    ? "Right"
                    : "Perspective";

        if (ImGui::Button(viewName)) {
            ImGui::OpenPopup("ViewportCameraPopup");
        }

        draw_layers(state);
        draw_display(state, streamer);
        draw_camera(state);

        return actions;
    }

}
