#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Transform3x4.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace core::world::xray
{
    inline constexpr std::uint32_t InvalidIndex = 0xFFFFFFFFu;

    struct Portal final
    {
        std::uint16_t frontSector = 0;
        std::uint16_t backSector = 0;
        std::array<math::Vector3, 6> vertices{};
        std::uint32_t vertexCount = 0;
    };

    struct Sector final
    {
        std::uint32_t rootVisual = InvalidIndex;
        std::vector<std::uint16_t> portals;
    };

    struct SlideWindow final
    {
        std::uint32_t offset = 0;
        std::uint16_t triangleCount = 0;
        std::uint16_t vertexCount = 0;
    };

    struct LodVertex final
    {
        math::Vector3 position;
        float u = 0.0f;
        float v = 0.0f;
        std::uint32_t colour = 0;
        std::uint8_t sun = 0;
    };

    struct LodDefinition final
    {
        std::array<LodVertex, 32> vertices{};
    };

    struct TreeDefinition final
    {
        math::Transform3x4 transform;
        std::array<float, 5> lightingScale{};
        std::array<float, 5> lightingBias{};
    };

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

        GeometryReference fastGeometry;
        std::uint32_t lodIndex = InvalidIndex;
        std::uint32_t treeIndex = InvalidIndex;
        std::uint32_t slideWindowIndex = InvalidIndex;

        std::vector<std::uint32_t>
            childVisuals;
    };

    struct LevelData final
    {
        std::filesystem::path levelDirectory;
        std::filesystem::path levelFile;
        std::filesystem::path geometryFile;
        std::filesystem::path secondaryGeometryFile;

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

        std::vector<Portal> portals;
        std::vector<Sector> sectors;
        std::vector<LodDefinition> lods;
        std::vector<TreeDefinition> trees;
        std::vector<std::vector<SlideWindow>> slideWindows;
        std::vector<VertexBuffer> secondaryVertexBuffers;
        std::vector<IndexBuffer> secondaryIndexBuffers;
    };
}
