#pragma once

#include "WorldEditor/WorldGenerator/MapLayout.h"
#include "Editor/Viewport/ViewportLayers.h"

#include "imgui.h"

#include <DirectXMath.h>

namespace editor::viewport {

    void draw_map_layout_overlay(
        const world_editor::world_generator::MapLayout& layout,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection,
        const ViewportLayers& layers
    );

}
