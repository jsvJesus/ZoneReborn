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

    struct PrimitivesGroup {
        std::uint32_t startIndex = 0;
        std::uint32_t indexCount = 0;
        std::uint32_t startVertex = 0;
        std::uint32_t vertexCount = 0;
    };

    struct PrimitivesMesh {
        std::vector<PrimitivesVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<PrimitivesGroup> groups;

        [[nodiscard]] bool valid() const noexcept;
    };

    [[nodiscard]] PrimitivesMesh load_primitives_mesh(
        const std::filesystem::path& path
    );

}
