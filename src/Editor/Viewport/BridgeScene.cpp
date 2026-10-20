#include "Editor/Viewport/BridgeScene.h"

#include "WorldEditor/Road/RoadCrossings.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace editor::viewport {

    namespace {

        constexpr float woodLength = 19.7565f;
        constexpr float woodCenterX = -0.2071f;
        constexpr float metalLength = 5.0f;
        constexpr float metalMinX = -0.0017f;

    }

    BridgeScene build_bridge_scene(
        const world_editor::road::RoadGraph& graph
    ) {
        BridgeScene scene;
        scene.assetPaths = {
            "models/props/bridges/wood_bridge_old/wood_bridge_old.primitives",
            "models/props/bridges/metal_bridge/metal_bridge_wall_part1.primitives"
        };

        for (const auto& crossing :
             world_editor::road::find_crossings(graph)) {
            const auto& route = graph.edges[crossing.edgeIndex].route;
            const auto& first = route[crossing.firstPoint];
            const auto& last = route[crossing.lastPoint];
            const float dx = last.x - first.x;
            const float dz = last.z - first.z;
            const float length = std::hypot(dx, dz);

            if (length < 1.0f) {
                continue;
            }

            const float forwardX = dx / length;
            const float forwardZ = dz / length;
            const float yaw = std::atan2(-dz, dx);
            const float deckHeight = (first.y + last.y) * 0.5f;

            if (crossing.kind ==
                world_editor::road::CrossingKind::WoodBridge) {
                engine::renderer::VegetationDrawInstance bridge;
                bridge.assetIndex = 0;
                bridge.x = (first.x + last.x) * 0.5f -
                    forwardX * woodCenterX * length / woodLength;
                bridge.y = deckHeight + 0.55f;
                bridge.z = (first.z + last.z) * 0.5f -
                    forwardZ * woodCenterX * length / woodLength;
                bridge.yawRadians = yaw;
                bridge.scaleX = length / woodLength;
                scene.instances.push_back(bridge);
                continue;
            }

            const auto sections = static_cast<std::uint32_t>(
                std::max(1.0f, std::ceil(length / metalLength))
            );
            const float sectionLength = length /
                static_cast<float>(sections);

            for (std::uint32_t section = 0; section < sections; ++section) {
                const float start =
                    static_cast<float>(section) * sectionLength -
                    metalMinX * sectionLength / metalLength;
                engine::renderer::VegetationDrawInstance bridge;
                bridge.assetIndex = 1;
                bridge.x = first.x + forwardX * start;
                bridge.y = deckHeight + 0.27f;
                bridge.z = first.z + forwardZ * start;
                bridge.yawRadians = yaw;
                bridge.scaleX = sectionLength / metalLength;
                scene.instances.push_back(bridge);
            }
        }

        return scene;
    }

}
