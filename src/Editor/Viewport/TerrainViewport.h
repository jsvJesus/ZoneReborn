#pragma once

#include "Engine/Renderer/TerrainRenderer.h"
#include "WorldEditor/Terrain/TerrainData.h"

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

    private:
        engine::renderer::TerrainRenderer renderer_;

        float targetX_ = 0.0f;
        float targetY_ = 0.0f;
        float targetZ_ = 0.0f;

        float yaw_ = 0.75f;
        float pitch_ = 0.7f;
        float distance_ = 1600.0f;

        bool wireframe_ = false;
    };

}
