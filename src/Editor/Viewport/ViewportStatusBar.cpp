#include "Editor/Viewport/ViewportStatusBar.h"

#include "Editor/Viewport/TerrainWorldStreamer.h"

#include "imgui.h"

namespace editor::viewport {

    void draw_viewport_status_bar(
        const engine::renderer::TerrainRenderStats& stats,
        const TerrainWorldStreamer* streamer,
        const float cameraDistance
    ) {
        ImGui::Separator();

        ImGui::TextDisabled(
            "FPS %.0f  |  Camera %.0f m",
            ImGui::GetIO().Framerate,
            cameraDistance
        );

        ImGui::SameLine();

        ImGui::TextDisabled(
            "|  Chunks %u/%u",
            stats.visibleChunks,
            stats.chunks
        );

        ImGui::SameLine();

        ImGui::TextDisabled(
            "|  Draw %u  |  Triangles %llu",
            stats.drawCalls,
            static_cast<unsigned long long>(stats.triangles)
        );

        if (streamer) {
            ImGui::SameLine();

            if (streamer->paused()) {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.48f, 0.42f, 1.0f),
                    "| STREAMING PAUSED"
                );

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "%s",
                        streamer->error().c_str()
                    );
                }
            } else {
                ImGui::TextDisabled(
                    "|  Stream %llu/%u",
                    static_cast<unsigned long long>(
                        streamer->loaded_count()
                    ),
                    streamer->total_count()
                );
            }
        }
    }

}
