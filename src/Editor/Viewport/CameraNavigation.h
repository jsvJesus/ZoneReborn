#pragma once

#include "imgui.h"
#include "Editor/Viewport/ViewportToolbar.h"

namespace editor::viewport {

    struct CameraNavigationState {
        float& targetX;
        float& targetY;
        float& targetZ;

        float& yaw;
        float& pitch;
        float& distance;
        CameraView view;
    };

    struct CameraNavigationSettings {
        float orbitSpeed = 1.0f;
        float panSpeed = 1.0f;
        float zoomSpeed = 1.0f;
    };

    void update_camera_navigation(
        CameraNavigationState camera,
        const CameraNavigationSettings& settings,
        const ImGuiIO& io
    );

}
