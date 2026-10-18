#include "Editor/Viewport/VegetationOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace editor::viewport {

    namespace {

        [[nodiscard]] bool project(
            const world_editor::vegetation::VegetationInstance&
                instance,
            DirectX::FXMMATRIX viewProjection,
            const ImVec2& origin,
            const ImVec2& size,
            ImVec2& output
        ) {
            const auto position = DirectX::XMVectorSet(
                instance.x,
                instance.y + 1.5f,
                instance.z,
                1.0f
            );

            DirectX::XMFLOAT4 clip{};

            DirectX::XMStoreFloat4(
                &clip,
                DirectX::XMVector4Transform(
                    position,
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

            const float normalizedX =
                clip.x * inverseW;

            const float normalizedY =
                clip.y * inverseW;

            if (!std::isfinite(normalizedX) ||
                !std::isfinite(normalizedY) ||
                normalizedX < -1.0f ||
                normalizedX > 1.0f ||
                normalizedY < -1.0f ||
                normalizedY > 1.0f) {
                return false;
            }

            output.x = origin.x +
                (normalizedX * 0.5f + 0.5f) *
                size.x;

            output.y = origin.y +
                (0.5f - normalizedY * 0.5f) *
                size.y;

            return true;
        }

    }

    void draw_vegetation_overlay(
        const world_editor::vegetation::VegetationLayer& layer,
        ImDrawList* drawList,
        const ImVec2& imagePosition,
        const ImVec2& imageSize,
        DirectX::FXMMATRIX viewProjection
    ) {
        if (!drawList || !layer.valid()) {
            return;
        }

        constexpr std::size_t maxMarkers = 6000;

        const std::size_t step = (std::max)(
            std::size_t{1},
            (
                layer.instances.size() +
                maxMarkers - 1
            ) / maxMarkers
        );

        drawList->PushClipRect(
            imagePosition,
            ImVec2(
                imagePosition.x + imageSize.x,
                imagePosition.y + imageSize.y
            ),
            true
        );

        for (std::size_t i = 0;
             i < layer.instances.size();
             i += step) {
            const auto& instance =
                layer.instances[i];

            ImU32 color = IM_COL32(
                75, 175, 95, 175
            );

            float radius = 1.8f;

            switch (instance.kind) {
            case world_editor::vegetation::VegetationKind::Grass:
                color = IM_COL32(75, 175, 95, 170);
                radius = 1.5f;
                break;

            case world_editor::vegetation::VegetationKind::Bush:
                color = IM_COL32(188, 202, 90, 210);
                radius = 2.4f;
                break;

            case world_editor::vegetation::VegetationKind::Tree:
                color = IM_COL32(78, 224, 156, 235);
                radius = 3.4f;
                break;
            }

            ImVec2 screen;

            if (!project(
                instance,
                viewProjection,
                imagePosition,
                imageSize,
                screen
            )) {
                continue;
            }

            drawList->AddCircleFilled(
                screen,
                radius,
                color,
                5
            );
        }

        drawList->PopClipRect();
    }

}
