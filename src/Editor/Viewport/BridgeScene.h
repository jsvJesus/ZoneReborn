#pragma once

#include "Engine/Renderer/VegetationRenderer.h"
#include "WorldEditor/Road/RoadGraph.h"

#include <string>
#include <vector>

namespace editor::viewport {

    struct BridgeScene {
        std::vector<std::string> assetPaths;
        std::vector<engine::renderer::VegetationDrawInstance> instances;
    };

    [[nodiscard]] BridgeScene build_bridge_scene(
        const world_editor::road::RoadGraph& graph
    );

}
