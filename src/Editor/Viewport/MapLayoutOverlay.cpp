#include "Editor/Viewport/MapLayoutOverlay.h"

#include <cmath>
#include <cstddef>
#include <cstdio>

namespace editor::viewport {

    namespace {

        constexpr ImU32 poiColor =
            IM_COL32(85, 226, 190, 255);

        constexpr ImU32 roadColor =
            IM_COL32(231, 184, 104, 255);

        constexpr ImU32 crossingColor =
            IM_COL32(255, 115, 96, 255);

        constexpr ImU32 exitColor =
            IM_COL32(255, 139, 96, 255);

        constexpr ImU32 shadowColor =
            IM_COL32(12, 20, 22, 230);

        [[nodiscard]] bool project(
            DirectX::FXMMATRIX viewProjection,
            const float x,
            const float y,
            const float z,
            const ImVec2& origin,
            const ImVec2& size,
            ImVec2& output
        ) {
            const DirectX::XMVECTOR world =
                DirectX::XMVectorSet(x, y, z, 1.0f);

            DirectX::XMFLOAT4 clip{};

            DirectX::XMStoreFloat4(
                &clip,
                DirectX::XMVector4Transform(
                    world,
                    viewProjection
                )
            );

            if (!std::isfinite(clip.w) ||
                clip.w <= 0.001f ||
                clip.z < 0.0f ||
                clip.z > clip.w) {
                return false;
            }

            const float inverseW = 1.0f / clip.w;

            output.x = origin.x + (
                clip.x * inverseW * 0.5f + 0.5f
            ) * size.x;

            output.y = origin.y + (
                0.5f - clip.y * inverseW * 0.5f
            ) * size.y;

            return std::isfinite(output.x) &&
                std::isfinite(output.y);
        }

        void draw_label(
            ImDrawList* drawList,
            const ImVec2& position,
            const char* label,
            const ImU32 color
        ) {
            const ImVec2 size = ImGui::CalcTextSize(label);

            const ImVec2 minimum(
                position.x + 10.0f,
                position.y - size.y - 12.0f
            );

            const ImVec2 maximum(
                minimum.x + size.x + 12.0f,
                minimum.y + size.y + 8.0f
            );

            drawList->AddRectFilled(
                minimum,
                maximum,
                shadowColor,
                4.0f
            );

            drawList->AddText(
                ImVec2(minimum.x + 6.0f, minimum.y + 4.0f),
                color,
                label
            );
        }

    }

    void draw_map_layout_overlay(
        const world_editor::world_generator::MapLayout& layout,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection,
        const ViewportLayers& layers
    ) {
        if (!drawList) {
            return;
        }

        drawList->PushClipRect(
            imagePosition,
            ImVec2(
                imagePosition.x + imageSize.x,
                imagePosition.y + imageSize.y
            ),
            true
        );

        if (layers.roads) {
            for (const auto& edge : layout.roads.edges) {
                if (edge.route.size() < 2) {
                    continue;
                }

                for (std::size_t i = 1; i < edge.route.size(); ++i) {
                    const auto& from = edge.route[i - 1];
                    const auto& to = edge.route[i];

                    ImVec2 a;
                    ImVec2 b;

                    if (!project(
                        viewProjection,
                        from.x, from.y + 3.0f, from.z,
                        imagePosition, imageSize, a
                    ) || !project(
                        viewProjection,
                        to.x, to.y + 3.0f, to.z,
                        imagePosition, imageSize, b
                    )) {
                        continue;
                    }

                    drawList->AddLine(a, b, shadowColor, 5.0f);
                    const bool crossing =
                        from.water == world_editor::biome::WaterKind::River ||
                        from.water == world_editor::biome::WaterKind::Lake ||
                        to.water == world_editor::biome::WaterKind::River ||
                        to.water == world_editor::biome::WaterKind::Lake;

                    drawList->AddLine(
                        a, b,
                        crossing ? crossingColor : roadColor,
                        2.5f
                    );
                }
            }
        }

        if (layers.poi) {
            for (const auto& poi : layout.pois) {
                const float halfWidth = poi.widthMeters * 0.5f;
                const float halfDepth = poi.depthMeters * 0.5f;

                const float corners[4][2] = {
                    {poi.centerX - halfWidth, poi.centerZ - halfDepth},
                    {poi.centerX + halfWidth, poi.centerZ - halfDepth},
                    {poi.centerX + halfWidth, poi.centerZ + halfDepth},
                    {poi.centerX - halfWidth, poi.centerZ + halfDepth}
                };

                ImVec2 projected[4];
                bool visible = true;

                for (int i = 0; i < 4; ++i) {
                    if (!project(
                        viewProjection,
                        corners[i][0],
                        poi.centerY + 3.0f,
                        corners[i][1],
                        imagePosition,
                        imageSize,
                        projected[i]
                    )) {
                        visible = false;
                        break;
                    }
                }

                if (visible) {
                    for (int i = 0; i < 4; ++i) {
                        drawList->AddLine(
                            projected[i],
                            projected[(i + 1) % 4],
                            shadowColor,
                            4.0f
                        );

                        drawList->AddLine(
                            projected[i],
                            projected[(i + 1) % 4],
                            poiColor,
                            2.0f
                        );
                    }
                }

                ImVec2 center;

                if (!project(
                    viewProjection,
                    poi.centerX,
                    poi.centerY + 4.0f,
                    poi.centerZ,
                    imagePosition,
                    imageSize,
                    center
                )) {
                    continue;
                }

                drawList->AddCircleFilled(
                    center,
                    6.0f,
                    shadowColor
                );

                drawList->AddCircleFilled(
                    center,
                    4.0f,
                    poiColor
                );

                char label[32]{};

                std::snprintf(
                    label,
                    sizeof(label),
                    "POI %u",
                    poi.id + 1
                );

                if (layers.labels) {
                    draw_label(drawList, center, label, poiColor);
                }
            }
        }

        if (layers.exits) {
            for (const auto& exit : layout.exits) {
                ImVec2 center;

                if (!project(
                    viewProjection,
                    exit.x,
                    exit.y + 4.0f,
                    exit.z,
                    imagePosition,
                    imageSize,
                    center
                )) {
                    continue;
                }

                const float radius = 9.0f;

                drawList->AddQuadFilled(
                    ImVec2(center.x, center.y - radius),
                    ImVec2(center.x + radius, center.y),
                    ImVec2(center.x, center.y + radius),
                    ImVec2(center.x - radius, center.y),
                    exitColor
                );

                char label[32]{};

                std::snprintf(
                    label,
                    sizeof(label),
                    "EXIT %u",
                    exit.id + 1
                );

                if (layers.labels) {
                    draw_label(drawList, center, label, exitColor);
                }
            }

            drawList->PopClipRect();
        }
    }
}
