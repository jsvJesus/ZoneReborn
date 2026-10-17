
#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/Viewport/MapLayoutOverlay.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <memory>
#include <utility>

namespace editor::viewport {

    TerrainViewport::TerrainViewport(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : renderer_(device, context) {
    }

    void TerrainViewport::reset_camera() {
        targetX_ = homeX_;
        targetY_ = homeY_;
        targetZ_ = homeZ_;

        distance_ = homeDistance_;
        yaw_ = 0.75f;
        pitch_ = 0.7f;
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

        streamer_.reset();

        const auto [minHeight, maxHeight] =
            std::minmax_element(
                terrain.heights.begin(),
                terrain.heights.end()
            );

        const float width =
            (terrain.samplesX - 1) * terrain.spacingMeters;

        const float depth =
            (terrain.samplesZ - 1) * terrain.spacingMeters;

        homeX_ = terrain.originX + width * 0.5f;
        homeY_ = (*minHeight + *maxHeight) * 0.5f;
        homeZ_ = terrain.originZ + depth * 0.5f;

        homeDistance_ = (std::max)(width, depth) * 1.1f;

        reset_camera();
    }

    void TerrainViewport::open_streamed_world(
        const std::filesystem::path& directory
    )
    {
        auto newStreamer = std::make_unique<TerrainWorldStreamer>(directory);

        const auto& settings = newStreamer->settings();

        const float width =
            (settings.samplesX - 1) * settings.spacingMeters;

        const float depth =
            (settings.samplesZ - 1) * settings.spacingMeters;

        renderer_.clear_chunks();
        streamer_ = std::move(newStreamer);

        homeX_ = settings.originX + width * 0.5f;
        homeY_ = settings.baseHeightMeters;
        homeZ_ = settings.originZ + depth * 0.5f;

        homeDistance_ = (std::max)(width, depth) * 0.5f;

        reset_camera();
    }

    const world_editor::terrain::Settings&
        TerrainViewport::streaming_settings() const
    {
        if (!streamer_) {
            throw std::logic_error("Streaming world is not active");
        }

        return streamer_->settings();
    }

    void TerrainViewport::set_map_layout(
        const world_editor::world_generator::MapLayout* layout
    ) {
        if (layout) {
            mapLayout_ = *layout;
        } else {
            mapLayout_.reset();
        }
    }

    void TerrainViewport::draw() {
        ImGui::Begin("Terrain Viewport");

        ImGui::TextDisabled(
            "RMB: Orbit | MMB: Pan | Wheel: Zoom"
        );

        ImGui::Checkbox("Wireframe", &wireframe_);

        ImGui::SameLine();
        ImGui::Checkbox("Frustum Culling", &frustumCulling_);

        ImGui::SameLine();

        if (ImGui::Button("Reset Camera")) {
            reset_camera();
        }

        ImGui::SameLine();

        ImGui::Checkbox(
            "Map Layout Overlay",
            &showMapLayout_
        );

        ImGui::Checkbox("Automatic LOD", &automaticLod_);

        if (automaticLod_) {
            ImGui::SameLine();

            ImGui::SetNextItemWidth(160.0f);

            ImGui::SliderFloat(
                "LOD Distance",
                &lodDistance_,
                0.5f,
                3.0f,
                "%.2f"
            );
        } else {
            ImGui::SameLine();

            ImGui::SetNextItemWidth(160.0f);

            ImGui::SliderInt(
                "Fixed LOD",
                &fixedLod_,
                0,
                3
            );
        }

        renderer_.set_wireframe(wireframe_);
        renderer_.set_frustum_culling(frustumCulling_);
        renderer_.set_auto_lod(automaticLod_);

        renderer_.set_fixed_lod(
            static_cast<std::uint32_t>(fixedLod_)
        );

        renderer_.set_lod_distance(lodDistance_);

        if (streamer_)
        {
            ImGui::Separator();
            ImGui::TextUnformatted("Terrain Streaming");

            int radius = streamer_->radius();

            if (ImGui::SliderInt("Stream Radius", &radius, 1, 4)) {
                streamer_->set_radius(radius);
            }

            ImGui::Text(
                "Resident: %llu / %u | Loaded: %llu | Unloaded: %llu",
                static_cast<unsigned long long>(streamer_->loaded_count()),
                streamer_->total_count(),
                static_cast<unsigned long long>(streamer_->total_loads()),
                static_cast<unsigned long long>(streamer_->total_unloads())
            );

            if (streamer_->paused()) {
                ImGui::TextWrapped(
                    "Streaming stopped: %s",
                    streamer_->error().c_str()
                );
            }
        }

        const auto& stats = renderer_.stats();

        ImGui::Text(
            "Visible: %u | Culled: %u | Total: %u",
            stats.visibleChunks,
            stats.culledChunks,
            stats.chunks
        );

        ImGui::Text(
            "Chunks: %u | Draw calls: %u | Triangles: %llu",
            stats.chunks,
            stats.drawCalls,
            static_cast<unsigned long long>(stats.triangles)
        );

        ImGui::Text(
            "LOD0: %u | LOD1: %u | LOD2: %u | LOD3: %u",
            stats.lodCounts[0],
            stats.lodCounts[1],
            stats.lodCounts[2],
            stats.lodCounts[3]
        );

        const ImVec2 size = ImGui::GetContentRegionAvail();

        if (size.x >= 16.0f && size.y >= 16.0f) {
            const UINT width = static_cast<UINT>(
                (std::min)(size.x, 8192.0f)
            );

            const UINT height = static_cast<UINT>(
                (std::min)(size.y, 8192.0f)
            );

            renderer_.resize(width, height);

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

            if (streamer_)
            {
                streamer_->update(
                    targetX_,
                    targetZ_,
                    renderer_
                );
            }

            renderer_.render(
                view * projection,
                DirectX::XMFLOAT3(eyeX, eyeY, eyeZ)
            );

            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    renderer_.texture()
                ),
                size
            );

            if (showMapLayout_ && mapLayout_) {
                draw_map_layout_overlay(
                    *mapLayout_,
                    ImGui::GetWindowDrawList(),
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectSize(),
                    view * projection
                );
            }

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

                if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                    const float speed =
                        distance_ * 0.0015f;

                    const float rightX = -std::cos(yaw_);
                    const float rightZ = std::sin(yaw_);

                    const float upX = -std::sin(yaw_);
                    const float upZ = -std::cos(yaw_);

                    targetX_ -=
                        io.MouseDelta.x * rightX * speed;

                    targetZ_ -=
                        io.MouseDelta.x * rightZ * speed;

                    targetX_ -=
                        io.MouseDelta.y * upX * speed;

                    targetZ_ -=
                        io.MouseDelta.y * upZ * speed;
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
