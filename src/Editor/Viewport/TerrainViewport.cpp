
#include "Editor/Viewport/TerrainViewport.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace editor::viewport {

TerrainViewport::TerrainViewport(
    ID3D11Device* device,
    ID3D11DeviceContext* context
) : renderer_(device, context) {
}

void TerrainViewport::set_terrain(
    const world_editor::terrain::Heightfield& terrain
) {
    if (!terrain.valid()) {
        throw std::invalid_argument(
            "Invalid terrain heightfield"
        );
    }

    renderer_.set_mesh({
        terrain.samplesX,
        terrain.samplesZ,
        terrain.originX,
        terrain.originZ,
        terrain.spacingMeters,
        std::span<const float>(terrain.heights)
    });

    const auto [minHeight, maxHeight] =
        std::minmax_element(
            terrain.heights.begin(),
            terrain.heights.end()
        );

    const float width =
        (terrain.samplesX - 1) * terrain.spacingMeters;

    const float depth =
        (terrain.samplesZ - 1) * terrain.spacingMeters;

    targetX_ = terrain.originX + width * 0.5f;
    targetY_ = (*minHeight + *maxHeight) * 0.5f;
    targetZ_ = terrain.originZ + depth * 0.5f;

    distance_ = std::max(width, depth) * 1.1f;
    yaw_ = 0.75f;
    pitch_ = 0.7f;
}

void TerrainViewport::draw() {
    ImGui::Begin("Terrain Viewport");

    ImGui::TextDisabled(
        "Right mouse: orbit | Mouse wheel: zoom"
    );

    ImGui::Checkbox("Wireframe", &wireframe_);

    ImVec2 size = ImGui::GetContentRegionAvail();

    if (size.x >= 16.0f && size.y >= 16.0f) {
        const UINT width = static_cast<UINT>(
            std::min(size.x, 8192.0f)
        );

        const UINT height = static_cast<UINT>(
            std::min(size.y, 8192.0f)
        );

        renderer_.resize(width, height);
        renderer_.set_wireframe(wireframe_);

        const float aspect =
            static_cast<float>(width) /
            static_cast<float>(height);

        const float horizontal =
            std::cos(pitch_) * distance_;

        const float eyeX =
            targetX_ + std::sin(yaw_) * horizontal;

        const float eyeY =
            targetY_ + std::sin(pitch_) * distance_;

        const float eyeZ =
            targetZ_ + std::cos(yaw_) * horizontal;

        const auto eye = DirectX::XMVectorSet(
            eyeX, eyeY, eyeZ, 1.0f
        );

        const auto target = DirectX::XMVectorSet(
            targetX_, targetY_, targetZ_, 1.0f
        );

        const auto up = DirectX::XMVectorSet(
            0.0f, 1.0f, 0.0f, 0.0f
        );

        const auto view = DirectX::XMMatrixLookAtLH(
            eye,
            target,
            up
        );

        const auto projection =
            DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XMConvertToRadians(60.0f),
                aspect,
                0.5f,
                30000.0f
            );

        renderer_.render(view * projection);

        ImGui::Image(
            reinterpret_cast<ImTextureID>(
                renderer_.texture()
            ),
            size
        );

        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();

            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                yaw_ += io.MouseDelta.x * 0.005f;

                pitch_ = std::clamp(
                    pitch_ - io.MouseDelta.y * 0.005f,
                    0.1f,
                    1.48f
                );
            }

            if (io.MouseWheel != 0.0f) {
                distance_ = std::clamp(
                    distance_ *
                        std::pow(0.87f, io.MouseWheel),
                    25.0f,
                    30000.0f
                );
            }
        }
    }

    ImGui::End();
}

}
