#include "Editor/Viewport/MapLayoutPicking.h"

#include <array>
#include <cmath>
#include <limits>

namespace editor::viewport {

    namespace {

        using namespace world_editor;

        constexpr ImU32 selectedColor =
            IM_COL32(255, 226, 114, 255);

        constexpr ImU32 selectedShadow =
            IM_COL32(12, 20, 22, 235);

        [[nodiscard]] bool project(
            DirectX::FXMMATRIX viewProjection,
            const float x,
            const float y,
            const float z,
            const ImVec2& origin,
            const ImVec2& size,
            ImVec2& output
        ) {
            const auto position = DirectX::XMVectorSet(
                x, y, z, 1.0f
            );

            DirectX::XMFLOAT4 clip{};

            DirectX::XMStoreFloat4(
                &clip,
                DirectX::XMVector4Transform(
                    position,
                    viewProjection
                )
            );

            if (!std::isfinite(clip.x) ||
                !std::isfinite(clip.y) ||
                !std::isfinite(clip.z) ||
                !std::isfinite(clip.w) ||
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

        [[nodiscard]] bool project_poi(
            const settlement::POIPlacement& poi,
            const ImVec2& origin,
            const ImVec2& size,
            DirectX::FXMMATRIX viewProjection,
            std::array<ImVec2, 4>& corners
        ) {
            const float halfWidth = poi.widthMeters * 0.5f;
            const float halfDepth = poi.depthMeters * 0.5f;

            const std::array<std::array<float, 2>, 4> positions = {{
                {poi.centerX - halfWidth, poi.centerZ - halfDepth},
                {poi.centerX + halfWidth, poi.centerZ - halfDepth},
                {poi.centerX + halfWidth, poi.centerZ + halfDepth},
                {poi.centerX - halfWidth, poi.centerZ + halfDepth}
            }};

            for (std::size_t i = 0; i < 4; ++i) {
                if (!project(
                    viewProjection,
                    positions[i][0],
                    poi.centerY + 3.0f,
                    positions[i][1],
                    origin,
                    size,
                    corners[i]
                )) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool inside_quad(
            const ImVec2& point,
            const std::array<ImVec2, 4>& corners
        ) noexcept {
            bool positive = false;
            bool negative = false;

            for (std::size_t i = 0; i < 4; ++i) {
                const ImVec2& a = corners[i];
                const ImVec2& b = corners[(i + 1) % 4];

                const float cross =
                    (b.x - a.x) * (point.y - a.y) -
                    (b.y - a.y) * (point.x - a.x);

                positive |= cross > 0.0f;
                negative |= cross < 0.0f;

                if (positive && negative) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] float distance_squared(
            const ImVec2& a,
            const ImVec2& b
        ) noexcept {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

    }

    ui::MapSelection pick_map_layout(
        const world_editor::world_generator::MapLayout& layout,
        const ImVec2& mousePosition,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection,
        const ViewportLayers& layers
    ) {
        ui::MapSelection nearest;
        float nearestDistance = 16.0f * 16.0f;
        
        for (const auto& poi : layout.pois) {
            ImVec2 center{};

            if (!layers.poi) {
                continue;
            }

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

            const float distance = distance_squared(
                center,
                mousePosition
            );

            if (distance < nearestDistance) {
                nearestDistance = distance;

                nearest = {
                    ui::MapSelectionKind::POI,
                    poi.id
                };
            }
        }

        for (const auto& exit : layout.exits) {
            ImVec2 center{};

            if (!layers.exits) {
                continue;
            }

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

            const float distance = distance_squared(
                center,
                mousePosition
            );

            if (distance < nearestDistance) {
                nearestDistance = distance;

                nearest = {
                    ui::MapSelectionKind::Exit,
                    exit.id
                };
            }
        }

        if (nearest.valid()) {
            return nearest;
        }

        for (const auto& poi : layout.pois) {
            std::array<ImVec2, 4> corners{};

            if (!project_poi(
                poi,
                imagePosition,
                imageSize,
                viewProjection,
                corners
            )) {
                continue;
            }

            if (inside_quad(mousePosition, corners)) {
                return {
                    ui::MapSelectionKind::POI,
                    poi.id
                };
            }
        }

        return {};
    }

    void draw_map_selection(
        const world_editor::world_generator::MapLayout& layout,
        const ui::MapSelection& selection,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    ) {
        if (!drawList || !selection.valid()) {
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

        if (selection.kind == ui::MapSelectionKind::POI) {
            for (const auto& poi : layout.pois) {
                if (poi.id != selection.id) {
                    continue;
                }

                std::array<ImVec2, 4> corners{};

                if (project_poi(
                    poi,
                    imagePosition,
                    imageSize,
                    viewProjection,
                    corners
                )) {
                    for (std::size_t i = 0; i < 4; ++i) {
                        const ImVec2& a = corners[i];
                        const ImVec2& b = corners[(i + 1) % 4];

                        drawList->AddLine(
                            a, b, selectedShadow, 7.0f
                        );

                        drawList->AddLine(
                            a, b, selectedColor, 3.5f
                        );
                    }
                }

                ImVec2 center{};

                if (project(
                    viewProjection,
                    poi.centerX,
                    poi.centerY + 4.0f,
                    poi.centerZ,
                    imagePosition,
                    imageSize,
                    center
                )) {
                    drawList->AddCircle(
                        center, 13.0f,
                        selectedShadow, 0, 6.0f
                    );

                    drawList->AddCircle(
                        center, 13.0f,
                        selectedColor, 0, 2.5f
                    );
                }

                break;
            }
        } else if (selection.kind ==
            ui::MapSelectionKind::Exit) {

            for (const auto& exit : layout.exits) {
                if (exit.id != selection.id) {
                    continue;
                }

                ImVec2 center{};

                if (project(
                    viewProjection,
                    exit.x,
                    exit.y + 4.0f,
                    exit.z,
                    imagePosition,
                    imageSize,
                    center
                )) {
                    drawList->AddCircle(
                        center, 17.0f,
                        selectedShadow, 0, 7.0f
                    );

                    drawList->AddCircle(
                        center, 17.0f,
                        selectedColor, 0, 3.0f
                    );
                }

                break;
            }
        }

        drawList->PopClipRect();
    }

}
