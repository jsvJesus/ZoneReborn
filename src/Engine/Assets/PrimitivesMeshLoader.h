#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace engine::assets {

    struct PrimitivesVertex {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float u = 0.0f;
        float v = 0.0f;
    };

    struct PrimitivesMesh {
        std::vector<PrimitivesVertex> vertices;
        std::vector<std::uint32_t> indices;

        [[nodiscard]] bool valid() const noexcept;
    };

    [[nodiscard]] PrimitivesMesh load_primitives_mesh(
        const std::filesystem::path& path
    );

}
