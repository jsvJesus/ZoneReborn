#pragma once

#include "Editor/Viewport/ViewportLayers.h"

namespace editor::viewport {

    class TerrainWorldStreamer;

    struct ViewportToolbarState {
        ViewportLayers layers;

        bool showLayout = true;
        bool roadSurface = true;
        bool waterGuides = false;
        bool wireframe = false;
        bool frustumCulling = true;
        bool automaticLod = true;

        float lodDistance = 1.0f;
        int fixedLod = 0;

        float orbitSpeed = 1.0f;
        float panSpeed = 1.0f;
        float zoomSpeed = 1.0f;
        float fieldOfViewDegrees = 60.0f;
    };

    struct ViewportToolbarActions {
        bool resetCamera = false;
        bool focusSelection = false;
        bool topView = false;
        bool perspectiveView = false;
    };

    [[nodiscard]] ViewportToolbarActions draw_viewport_toolbar(
        ViewportToolbarState& state,
        bool hasSelection,
        TerrainWorldStreamer* streamer
    );

}
