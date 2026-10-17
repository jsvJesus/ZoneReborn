#include "Editor/Viewport/HydrologyOverlay.h"

#include <cmath>

namespace editor::viewport {

    namespace {

        constexpr ImU32 riverColor =
            IM_COL32(70, 165, 235, 230);

        constexpr ImU32 lakeColor =
            IM_COL32(55, 145, 225, 115);

        constexpr ImU32 lakeOutline =
            IM_COL32(90, 195, 255, 255);

        constexpr ImU32 marshColor =
            IM_COL32(112, 182, 132, 145);

        [[nodiscard]] bool project(
            DirectX::FXMMATRIX matrix,
            const world_editor::biome::WaterPoint& point,
            const ImVec2& origin,
            const ImVec2& size,
            ImVec2& result
        ) {
            const auto position = DirectX::XMVectorSet(
                point.x,
                point.y + 3.0f,
                point.z,
                1.0f
            );

            DirectX::XMFLOAT4 clip{};

            DirectX::XMStoreFloat4(
                &clip,
                DirectX::XMVector4Transform(position, matrix)
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

            result.x = origin.x +
                (clip.x * inverseW * 0.5f + 0.5f) *
                size.x;

            result.y = origin.y +
                (0.5f - clip.y * inverseW * 0.5f) *
                size.y;

            return std::isfinite(result.x) &&
                std::isfinite(result.y);
        }

    }

    void draw_hydrology_overlay(
        const world_editor::biome::HydrologyLayer& water,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    ) {
        if (!drawList || !water.valid()) {
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

        // River centerline.
        const auto draw_channel = [&](const std::vector<world_editor::biome::WaterPoint>& channel) {
            for (std::size_t i = 1; i < channel.size(); ++i) {
                ImVec2 start{};
                ImVec2 end{};

                if (!project(
                    viewProjection,
                    channel[i - 1],
                    imagePosition,
                    imageSize,
                    start
                ) || !project(
                    viewProjection,
                    channel[i],
                    imagePosition,
                    imageSize,
                    end
                )) {
                    continue;
                }

                drawList->AddLine(
                    start,
                    end,
                    IM_COL32(15, 40, 65, 230),
                    7.0f
                );

                drawList->AddLine(
                    start,
                    end,
                    riverColor,
                    4.0f
                );
            }
        };

        draw_channel(water.river);

        for (const auto& branch : water.tributaries) {
            draw_channel(branch);
        }

        // Schematic lake areas.
        constexpr int segments = 24;
        constexpr float pi = 3.14159265358979323846f;

        for (const auto& lake : water.lakes) {
            ImVec2 points[segments]{};
            bool visible = true;

            for (int i = 0; i < segments; ++i) {
                const float angle =
                    2.0f * pi * static_cast<float>(i) /
                    static_cast<float>(segments);

                world_editor::biome::WaterPoint point{
                    lake.center.x +
                        std::cos(angle) * lake.radiusMeters,
                    lake.center.y,
                    lake.center.z +
                        std::sin(angle) * lake.radiusMeters
                };

                if (!project(
                    viewProjection,
                    point,
                    imagePosition,
                    imageSize,
                    points[i]
                )) {
                    visible = false;
                    break;
                }
            }

            if (!visible) {
                continue;
            }

            drawList->AddConvexPolyFilled(
                points,
                segments,
                lakeColor
            );

            drawList->AddPolyline(
                points,
                segments,
                lakeOutline,
                ImDrawFlags_Closed,
                2.0f
            );
        }

        // Sparse marsh visualization.
        for (const auto& marker : water.marshMarkers) {
            ImVec2 position{};

            if (!project(
                viewProjection,
                marker,
                imagePosition,
                imageSize,
                position
            )) {
                continue;
            }

            drawList->AddCircleFilled(
                position,
                2.5f,
                marshColor
            );
        }

        drawList->PopClipRect();
    }

}
