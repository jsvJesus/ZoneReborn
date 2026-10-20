#include "Editor/Viewport/TerrainViewport.h"

#include "Editor/Core/Paths.h"
#include "Editor/Viewport/BridgeScene.h"
#include "Editor/Viewport/CameraNavigation.h"
#include "Editor/Viewport/HydrologyOverlay.h"
#include "Editor/Viewport/MapLayoutOverlay.h"
#include "Editor/Viewport/MapLayoutPicking.h"
#include "Editor/Viewport/VegetationOverlay.h"
#include "Editor/Viewport/ViewportStatusBar.h"
#include "Editor/Viewport/ViewportToolbar.h"

#include "WorldEditor/Biome/BiomeStorage.h"
#include "WorldEditor/Biome/WaterSurface.h"
#include "WorldEditor/Road/RoadSurface.h"
#include "WorldEditor/WorldGenerator/MapLayoutFingerprint.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

        const auto fingerprint =
            world_editor::biome::terrain_source_fingerprint(terrain);

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

        // reset
        terrainGrid_ = world_generator::layer_grid(terrain);
        terrainFingerprint_ = fingerprint;
        sourceFingerprint_ = world_editor::world_generator::layout_source_fingerprint(
            terrainFingerprint_, nullptr
        );
        ++revisions_.terrain;
        ++revisions_.hydrology;
        ++revisions_.layout;
        mapLayout_.reset();
        renderer_.clear_bridge_scene();
        hydrology_.reset();
        surfaceTerrain_ = terrain;
        clear_biomes();
        selection_ = {};

        reset_camera();
    }

    void TerrainViewport::open_streamed_world(
        const std::filesystem::path& directory,
        const world_editor::terrain::Heightfield& source
    ) {
        auto newStreamer =
            std::make_unique<TerrainWorldStreamer>(directory);

        const auto& settings = newStreamer->settings();

        if (!source.valid() || source.samplesX != settings.samplesX ||
            source.samplesZ != settings.samplesZ ||
            source.originX != settings.originX ||
            source.originZ != settings.originZ ||
            source.spacingMeters != settings.spacingMeters) {
            throw std::invalid_argument("Streaming Terrain source does not match World");
        }

        const auto fingerprint =
            world_editor::biome::terrain_source_fingerprint(source);

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

        // reset
        terrainGrid_ = world_generator::layer_grid(source);
        terrainFingerprint_ = fingerprint;
        sourceFingerprint_ = world_editor::world_generator::layout_source_fingerprint(
            terrainFingerprint_, nullptr
        );
        ++revisions_.terrain;
        ++revisions_.hydrology;
        ++revisions_.layout;
        mapLayout_.reset();
        renderer_.clear_bridge_scene();
        hydrology_.reset();
        surfaceTerrain_ = source;
        clear_biomes();
        selection_ = {};

        reset_camera();
    }

    const world_editor::terrain::Settings&
        TerrainViewport::streaming_settings() const {
        if (!streamer_) {
            throw std::logic_error(
                "Streaming world is not active"
            );
        }

        return streamer_->settings();
    }

    void TerrainViewport::set_map_layout(
        const world_editor::world_generator::MapLayout* layout
    ) {
        if (layout && layout->sourceFingerprint != sourceFingerprint_) {
            throw std::invalid_argument("Map Layout source does not match Terrain or Hydrology");
        }

        if (layout) {
            const auto packs = editor::paths::packs_directory();
            const auto roadSurface = world_editor::road::build_road_surface(
                layout->roads,
                packs / "models/roads_segments/dirt01.primitives"
            );

            renderer_.set_road_surface(
                roadSurface,
                {
                    packs / "models/roads_segments/road_dirt0048.dds",
                    packs / "models/roads_segments/road_dirt0048_norm.dds"
                }
            );

            const auto bridges = build_bridge_scene(layout->roads);

            renderer_.set_bridge_scene(
                packs,
                bridges.assetPaths,
                bridges.instances
            );
            mapLayout_ = *layout;
        } else {
            // reset
            mapLayout_.reset();
            renderer_.clear_road_surface();
            renderer_.clear_bridge_scene();
        }

        ++revisions_.layout;
        clear_biomes();
        selection_ = {};
    }

    void TerrainViewport::update_map_layout_seeds(
        const world_editor::world_generator::GenerationSeeds& seeds
    ) {
        if (mapLayout_) {
            mapLayout_->settings.seeds.biome = seeds.biome;
            mapLayout_->settings.seeds.vegetation = seeds.vegetation;
            mapLayout_->settings.seeds.decoration = seeds.decoration;
        }
    }

    void TerrainViewport::set_hydrology(
        const world_editor::biome::HydrologyLayer* water
    ) {
        if (water && world_generator::layer_grid(*water) != terrainGrid_) {
            throw std::invalid_argument("Hydrology grid does not match Terrain");
        }

        const auto fingerprint = world_editor::world_generator::layout_source_fingerprint(
            terrainFingerprint_, water
        );

        if (water) {
            if (!surfaceTerrain_) {
                throw std::logic_error("Water surface requires Terrain");
            }

            const auto surfaces = world_editor::biome::build_water_surfaces(
                *water, *surfaceTerrain_
            );
            const auto textures = editor::paths::packs_directory() /
                "textures";

            renderer_.set_water_surfaces(
                surfaces.openWater,
                {
                    textures / "water_norm/water.dds",
                    textures / "water_norm/water_normal.dds",
                    {0.32f, 0.65f, 0.83f},
                    true
                },
                surfaces.marsh,
                {
                    textures / "landscape/water/swamp_water.dds",
                    textures / "landscape/water/swamp_water_norm.dds",
                    {0.48f, 0.66f, 0.55f},
                    true
                }
            );
            hydrology_ = *water;
        } else {
            // reset
            hydrology_.reset();
            renderer_.clear_water_surfaces();
        }

        sourceFingerprint_ = fingerprint;
        ++revisions_.hydrology;
        set_map_layout(nullptr);
    }

    void TerrainViewport::set_biomes(
        const world_editor::biome::BiomeLayer& layer
    ) {
        if (!layer.valid() || world_generator::layer_grid(layer) != terrainGrid_) {
            throw std::invalid_argument(
                "Invalid Biome Layer"
            );
        }

        const std::filesystem::path textures =
            editor::paths::packs_directory() /
            "textures";

        const std::array<
            engine::renderer::TerrainMaterialPaths,
            5
        > materials = {{
            {
                textures /
                    "landscape/forest/grass_03a.dds",

                textures /
                    "landscape/forest/grass_03a_norm.dds"
            },
            {
                textures /
                    "landscape/field/soil_dirt.dds",

                textures /
                    "landscape/field/soil_dirt_norm.dds"
            },
            {
                textures /
                    "landscape/water/sand_river01.dds",

                textures /
                    "landscape/water/sand_river01_norm.dds"
            },
            {
                textures /
                    "materials/stone/rocks.dds",

                textures /
                    "materials/stone/rocks_norm.dds"
            },
            {
                textures /
                    "landscape/water/swamp_grassy.dds",

                textures /
                    "landscape/water/swamp_grassy_norm.dds"
            }
        }};

        engine::renderer::TerrainMaterialMask mask;

        mask.samplesX = layer.samplesX;
        mask.samplesZ = layer.samplesZ;

        mask.originX = layer.originX;
        mask.originZ = layer.originZ;

        mask.spacingMeters = layer.spacingMeters;

        mask.textureScaleMeters =
            layer.settings.textureScaleMeters;

        mask.weights = std::span<const std::uint8_t>(
            layer.materialWeights.data(),
            layer.materialWeights.size()
        );

        renderer_.set_terrain_materials(
            mask,
            materials
        );
        ++revisions_.biomes;
        set_vegetation(nullptr);
    }

    void TerrainViewport::clear_biomes() {
        ++revisions_.biomes;
        renderer_.clear_terrain_materials();
        set_vegetation(nullptr);
    }

    void TerrainViewport::set_vegetation(
        const world_editor::vegetation::VegetationLayer* layer
    ) {
        if (!layer) {
            vegetation_.reset();
            renderer_.clear_vegetation_scene();
            return;
        }

        if (!layer->valid() || !has_biomes() ||
            world_generator::layer_grid(*layer) != terrainGrid_) {
            throw std::invalid_argument(
                "Invalid Vegetation Layer"
            );
        }

        std::vector<
            engine::renderer::VegetationDrawInstance
        > instances;

        instances.reserve(
            layer->instances.size()
        );

        for (const auto& source : layer->instances) {
            engine::renderer::VegetationDrawInstance instance;

            instance.assetIndex = source.assetIndex;

            instance.x = source.x;
            instance.y = source.y;
            instance.z = source.z;

            instance.yawRadians = source.yawRadians;
            instance.scale = source.scale;

            instance.kind = static_cast<std::uint32_t>(
                source.kind
            );

            instances.push_back(instance);
        }

        renderer_.set_vegetation_scene(
            editor::paths::packs_directory(),
            layer->assetPaths,
            instances
        );

        vegetation_ = *layer;
    }

    void TerrainViewport::select(
        const ui::MapSelection selection
    ) noexcept {
        if (!mapLayout_ || !selection.valid()) {
            selection_ = {};
            return;
        }

        bool exists = false;

        if (selection.kind == ui::MapSelectionKind::POI) {
            exists = std::any_of(
                mapLayout_->pois.begin(),
                mapLayout_->pois.end(),
                [selection](const auto& poi) {
                    return poi.id == selection.id;
                }
            );
        } else if (selection.kind == ui::MapSelectionKind::Exit) {
            exists = std::any_of(
                mapLayout_->exits.begin(),
                mapLayout_->exits.end(),
                [selection](const auto& exit) {
                    return exit.id == selection.id;
                }
            );
        }

        selection_ = exists
            ? selection
            : ui::MapSelection{};
    }

    void TerrainViewport::focus_selection() {
        if (!mapLayout_ || !selection_.valid()) {
            return;
        }

        if (selection_.kind == ui::MapSelectionKind::POI) {
            for (const auto& poi : mapLayout_->pois) {
                if (poi.id != selection_.id) {
                    continue;
                }

                targetX_ = poi.centerX;
                targetY_ = poi.centerY;
                targetZ_ = poi.centerZ;

                distance_ = std::clamp(
                    (std::max)(
                        poi.widthMeters,
                        poi.depthMeters
                    ) * 2.5f,
                    120.0f,
                    30000.0f
                );

                return;
            }
        }

        if (selection_.kind == ui::MapSelectionKind::Exit) {
            for (const auto& exit : mapLayout_->exits) {
                if (exit.id != selection_.id) {
                    continue;
                }

                targetX_ = exit.x;
                targetY_ = exit.y;
                targetZ_ = exit.z;

                distance_ = std::clamp(
                    homeDistance_ * 0.22f,
                    250.0f,
                    30000.0f
                );

                return;
            }
        }
    }

    void TerrainViewport::draw() {
        if (!ImGui::Begin("Terrain Viewport")) {
            ImGui::End();
            return;
        }

        const ViewportToolbarActions actions =
            draw_viewport_toolbar(
                toolbar_,
                selection_.valid(),
                streamer_.get()
            );

        if (actions.resetCamera) {
            reset_camera();
        }

        if (actions.focusSelection) {
            focus_selection();
        }

        if (actions.topView) {
            pitch_ = 1.48f;
            yaw_ = 0.0f;
        }

        if (actions.perspectiveView) {
            pitch_ = 0.7f;
            yaw_ = 0.75f;
        }

        renderer_.set_wireframe(toolbar_.wireframe);
        renderer_.set_frustum_culling(
            toolbar_.frustumCulling
        );

        renderer_.set_auto_lod(toolbar_.automaticLod);

        renderer_.set_fixed_lod(
            static_cast<std::uint32_t>(toolbar_.fixedLod)
        );

        renderer_.set_lod_distance(
            toolbar_.lodDistance
        );

        ImVec2 available = ImGui::GetContentRegionAvail();

        const float statusHeight =
            ImGui::GetFrameHeightWithSpacing() + 6.0f;

        available.y = (std::max)(
            0.0f,
            available.y - statusHeight
        );

        if (available.x >= 16.0f &&
            available.y >= 16.0f) {

            const UINT width = static_cast<UINT>(
                (std::min)(available.x, 8192.0f)
            );

            const UINT height = static_cast<UINT>(
                (std::min)(available.y, 8192.0f)
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
                eyeX,
                eyeY,
                eyeZ,
                1.0f
            );

            const auto target = DirectX::XMVectorSet(
                targetX_,
                targetY_,
                targetZ_,
                1.0f
            );

            const auto up = DirectX::XMVectorSet(
                0.0f,
                1.0f,
                0.0f,
                0.0f
            );

            const auto view = DirectX::XMMatrixLookAtLH(
                eye,
                target,
                up
            );

            const auto projection =
                DirectX::XMMatrixPerspectiveFovLH(
                    DirectX::XMConvertToRadians(
                        toolbar_.fieldOfViewDegrees
                    ),
                    aspect,
                    0.5f,
                    30000.0f
                );

            if (streamer_) {
                streamer_->update(
                    targetX_,
                    targetZ_,
                    renderer_
                );
            }

            renderer_.set_road_surface_visible(toolbar_.roadSurface);
            renderer_.set_water_surfaces_visible(toolbar_.layers.water);

            renderer_.render(
                view * projection,
                DirectX::XMFLOAT3(
                    eyeX,
                    eyeY,
                    eyeZ
                )
            );

            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    renderer_.texture()
                ),
                ImVec2(
                    static_cast<float>(width),
                    static_cast<float>(height)
                )
            );

            const bool imageHovered = ImGui::IsItemHovered();
            const ImVec2 imagePosition = ImGui::GetItemRectMin();
            const ImVec2 imageSize = ImGui::GetItemRectSize();

            if (vegetationVisible_ && vegetation_) {
                draw_vegetation_overlay(
                    *vegetation_,
                    ImGui::GetWindowDrawList(),
                    imagePosition,
                    imageSize,
                    view * projection
                );
            }

            if (hydrology_ && toolbar_.waterGuides) {
                draw_hydrology_overlay(
                    *hydrology_,
                    ImGui::GetWindowDrawList(),
                    imagePosition,
                    imageSize,
                    view * projection
                );
            }

            if (toolbar_.showLayout && mapLayout_) {
                if (selectionEnabled_ && imageHovered &&
                    ImGui::IsMouseClicked(
                        ImGuiMouseButton_Left
                    )) {
                    select(pick_map_layout(
                        *mapLayout_,
                        ImGui::GetMousePos(),
                        imagePosition,
                        imageSize,
                        view * projection,
                        toolbar_.layers
                    ));
                }

                draw_map_layout_overlay(
                    *mapLayout_,
                    ImGui::GetWindowDrawList(),
                    imagePosition,
                    imageSize,
                    view * projection,
                    toolbar_.layers
                );

                const bool selectionVisible =
                    (selection_.kind ==
                        ui::MapSelectionKind::POI &&
                        toolbar_.layers.poi) ||
                    (selection_.kind ==
                        ui::MapSelectionKind::Exit &&
                        toolbar_.layers.exits);

                if (selectionVisible) {
                    draw_map_selection(
                        *mapLayout_,
                        selection_,
                        ImGui::GetWindowDrawList(),
                        imagePosition,
                        imageSize,
                        view * projection
                    );
                }
            }

            if (imageHovered) {
                ImGuiIO& io = ImGui::GetIO();

                if (!io.WantTextInput) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
                        reset_camera();
                    }

                    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                        focus_selection();
                    }
                }

                update_camera_navigation(
                    {
                        targetX_,
                        targetY_,
                        targetZ_,
                        yaw_,
                        pitch_,
                        distance_
                    },
                    {
                        toolbar_.orbitSpeed,
                        toolbar_.panSpeed,
                        toolbar_.zoomSpeed
                    },
                    io
                );
            }
        }

        draw_viewport_status_bar(
            renderer_.stats(),
            streamer_.get(),
            distance_
        );

        ImGui::End();
    }

}
