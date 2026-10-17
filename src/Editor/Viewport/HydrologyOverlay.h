#pragma once

#include "WorldEditor/Biome/Hydrology.h"

#include "imgui.h"

#include <DirectXMath.h>

namespace editor::viewport {

    void draw_hydrology_overlay(
        const world_editor::biome::HydrologyLayer& water,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    );

}
