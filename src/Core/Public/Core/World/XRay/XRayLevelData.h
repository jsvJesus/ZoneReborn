#pragma once

#include "Core/Math/Vector3.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace core::world::xray
{
    struct ShaderReference final
    {
        std::string renderer;

        std::vector<std::string>
            textures;
    };

    struct VertexElement final
    {
        std::uint16_t stream = 0;
        std::uint16_t offset = 0;
        std::uint8_t type = 0;
        std::uint8_t method = 0;
        std::uint8_t usage = 0;
        std::uint8_t usageIndex = 0;
    };

    struct VertexBuffer final
    {
        std::vector<VertexElement> elements;

        std::uint64_t dataOffset = 0;
        std::uint32_t vertexCount = 0;
        std::uint32_t stride = 0;
    };

    struct IndexBuffer final
    {
        std::uint64_t dataOffset = 0;
        std::uint32_t indexCount = 0;
    };

    struct GeometryReference final
    {
        std::uint32_t vertexBuffer = 0;
        std::uint32_t vertexBase = 0;
        std::uint32_t vertexCount = 0;

        std::uint32_t indexBuffer = 0;
        std::uint32_t indexBase = 0;
        std::uint32_t indexCount = 0;

        bool valid = false;
    };

    struct Visual final
    {
        std::uint8_t formatVersion = 0;
        std::uint8_t type = 0;
        std::uint16_t shaderId = 0;

        math::Vector3 boundsMinimum;
        math::Vector3 boundsMaximum;

        GeometryReference geometry;

        std::vector<std::uint32_t>
            childVisuals;
    };

    struct LevelData final
    {
        std::filesystem::path levelDirectory;
        std::filesystem::path levelFile;
        std::filesystem::path geometryFile;

        std::uint16_t version = 0;
        std::uint16_t quality = 0;
        std::uint32_t shaderCount = 0;

        std::vector<ShaderReference>
            shaders;

        std::vector<VertexBuffer>
            vertexBuffers;

        std::vector<IndexBuffer>
            indexBuffers;

        std::vector<Visual>
            visuals;
    };
}
