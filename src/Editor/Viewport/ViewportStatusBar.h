#pragma once

#include "Engine/Renderer/TerrainRenderer.h"

namespace editor::viewport {

    class TerrainWorldStreamer;

    void draw_viewport_status_bar(
        const engine::renderer::TerrainRenderStats& stats,
        const TerrainWorldStreamer* streamer,
        float cameraDistance
    );

}
