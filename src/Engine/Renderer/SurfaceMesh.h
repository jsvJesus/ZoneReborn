#pragma once

#include <cstdint>
#include <vector>

namespace engine::renderer {

    struct SurfaceVertex {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
    };

    struct SurfaceMesh {
        std::vector<SurfaceVertex> vertices;
        std::vector<std::uint32_t> indices;
    };

}
