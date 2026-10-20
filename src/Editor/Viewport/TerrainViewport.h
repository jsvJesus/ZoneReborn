#pragma once

#include "Engine/Renderer/TerrainRenderer.h"

#include "WorldEditor/Terrain/TerrainData.h"
#include "WorldEditor/WorldGenerator/MapLayout.h"
#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Biome/BiomeGenerator.h"
#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include "Editor/UI/MapSelection.h"
#include "Editor/Viewport/TerrainWorldStreamer.h"
#include "Editor/Viewport/ViewportToolbar.h"
#include "Editor/WorldGenerator/LayerRevisions.h"

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
            const std::filesystem::path& directory,
            const world_editor::terrain::Heightfield& source
        );

        [[nodiscard]] const world_generator::LayerRevisions& layer_revisions() const noexcept {
            return revisions_;
        }

        [[nodiscard]] std::uint64_t terrain_fingerprint() const noexcept {
            return terrainFingerprint_;
        }

        [[nodiscard]] const world_generator::LayerGrid& terrain_grid() const noexcept {
            return terrainGrid_;
        }

        [[nodiscard]] std::uint64_t source_fingerprint() const noexcept {
            return sourceFingerprint_;
        }

        void update_map_layout_seeds(
            const world_editor::world_generator::GenerationSeeds& seeds
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

        void set_selection_enabled(bool enabled) noexcept {
            selectionEnabled_ = enabled;
        }

        void focus_selection();

        [[nodiscard]] CameraView camera_view() const noexcept {
            return toolbar_.cameraView;
        }

        void set_hydrology(
            const world_editor::biome::HydrologyLayer* water
        );

        void set_biomes(
            const world_editor::biome::BiomeLayer& layer
        );

        void clear_biomes();

        [[nodiscard]] bool has_biomes() const noexcept {
            return renderer_.has_terrain_materials();
        }

        [[nodiscard]]
            const world_editor::biome::HydrologyLayer*hydrology() const noexcept
        {
            return hydrology_ ? &*hydrology_ : nullptr;
        }

        void set_vegetation(
            const world_editor::vegetation::VegetationLayer* layer
        );

        void set_vegetation_visible(bool visible) noexcept {
            vegetationVisible_ = visible;
        }

        [[nodiscard]] bool has_vegetation() const noexcept {
            return vegetation_.has_value();
        }

        void set_vegetation_geometry_visible(bool visible) noexcept {
            renderer_.set_vegetation_geometry_visible(
                visible
            );
        }

        [[nodiscard]]
        const engine::renderer::VegetationRenderStats&
        vegetation_render_stats() const noexcept {
            return renderer_.vegetation_render_stats();
        }

        [[nodiscard]] const engine::renderer::VegetationRenderStats&
        bridge_render_stats() const noexcept {
            return renderer_.bridge_render_stats();
        }

    private:
        void reset_camera();

        world_generator::LayerRevisions revisions_;
        world_generator::LayerGrid terrainGrid_;
        std::uint64_t terrainFingerprint_ = 0;
        std::uint64_t sourceFingerprint_ = 0;

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

        ViewportToolbarState toolbar_;

        std::unique_ptr<TerrainWorldStreamer> streamer_;

        std::optional<world_editor::world_generator::MapLayout> mapLayout_;
        std::optional<world_editor::biome::HydrologyLayer> hydrology_;
        std::optional<world_editor::terrain::Heightfield> surfaceTerrain_;

        ui::MapSelection selection_;
        bool selectionEnabled_ = true;

        std::optional<world_editor::vegetation::VegetationLayer> vegetation_;
        bool vegetationVisible_ = true;
    };

}
