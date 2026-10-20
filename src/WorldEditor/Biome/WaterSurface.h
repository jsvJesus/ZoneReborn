#pragma once

#include "Engine/Renderer/SurfaceMesh.h"
#include "WorldEditor/Biome/Hydrology.h"
#include "WorldEditor/Terrain/TerrainData.h"

namespace world_editor::biome {

    struct WaterSurfaces {
        engine::renderer::SurfaceMesh openWater;
        engine::renderer::SurfaceMesh marsh;
    };

    [[nodiscard]] WaterSurfaces build_water_surfaces(
        const HydrologyLayer& water,
        const terrain::Heightfield& terrain
    );

}
