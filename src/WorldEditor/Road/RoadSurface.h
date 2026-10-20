#pragma once

#include "Engine/Renderer/SurfaceMesh.h"
#include "WorldEditor/Road/RoadGraph.h"

#include <filesystem>

namespace world_editor::road {

    [[nodiscard]] engine::renderer::SurfaceMesh build_road_surface(
        const RoadGraph& graph,
        const std::filesystem::path& segmentModel,
        float widthMeters = 8.0f
    );

}
