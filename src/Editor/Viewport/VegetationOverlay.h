#pragma once

#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include "imgui.h"

#include <DirectXMath.h>

namespace editor::viewport {

    void draw_vegetation_overlay(
        const world_editor::vegetation::VegetationLayer& layer,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    );

}
