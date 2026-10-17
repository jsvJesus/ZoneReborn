#pragma once

#include "Editor/UI/MapSelection.h"

#include "WorldEditor/WorldGenerator/MapLayout.h"
#include "Editor/Viewport/ViewportLayers.h"

#include "imgui.h"

#include <DirectXMath.h>

namespace editor::viewport {

    [[nodiscard]] ui::MapSelection pick_map_layout(
        const world_editor::world_generator::MapLayout& layout,
        const ImVec2& mousePosition,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection,
        const ViewportLayers& layers
    );

    void draw_map_selection(
        const world_editor::world_generator::MapLayout& layout,
        const ui::MapSelection& selection,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    );

}
