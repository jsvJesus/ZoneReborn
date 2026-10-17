#pragma once

#include "Engine/Renderer/TerrainRenderer.h"

#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"

#include "Editor/UI/MapSelection.h"
#include "Editor/Viewport/TerrainWorldStreamer.h"

#include <filesystem>
#include <memory>
#include <optional>

namespace editor::viewport {

    class TerrainViewport final {
    public:
        TerrainViewport(
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        void set_terrain(
            const world_editor::terrain::Heightfield& terrain
        );

        void draw();

        void open_streamed_world(
            const std::filesystem::path& directory
        );

        [[nodiscard]] bool is_streaming() const noexcept {
            return streamer_ != nullptr;
        }

        [[nodiscard]] const world_editor::terrain::Settings&
        streaming_settings() const;

        void set_map_layout(
            const world_editor::world_generator::MapLayout* layout
        );

        [[nodiscard]]
        const world_editor::world_generator::MapLayout*
        map_layout() const noexcept {
            return mapLayout_ ? &*mapLayout_ : nullptr;
        }

        [[nodiscard]] ui::MapSelection selection() const noexcept {
            return selection_;
        }

        void select(ui::MapSelection selection) noexcept;

        void focus_selection();

    private:
        void reset_camera();

        engine::renderer::TerrainRenderer renderer_;

        float targetX_ = 0.0f;
        float targetY_ = 0.0f;
        float targetZ_ = 0.0f;

        float homeX_ = 0.0f;
        float homeY_ = 0.0f;
        float homeZ_ = 0.0f;

        float yaw_ = 0.75f;
        float pitch_ = 0.7f;
        float distance_ = 1600.0f;
        float homeDistance_ = 1600.0f;

        float lodDistance_ = 1.0f;
        int fixedLod_ = 0;

        bool automaticLod_ = true;
        bool wireframe_ = false;
        bool frustumCulling_ = true;

        std::unique_ptr<TerrainWorldStreamer> streamer_;

        std::optional<world_editor::world_generator::MapLayout> mapLayout_;

        ui::MapSelection selection_;

        bool showMapLayout_ = true;
    };

}
