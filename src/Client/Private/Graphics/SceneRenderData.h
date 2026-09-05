#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Images/RgbaImage.h"
#include "Core/Math/Transform3x4.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace client::graphics
{
    enum class SceneAlphaMode : std::uint8_t
    {
        Opaque = 0,
        Cutout = 1,
        Blend = 2
    };

    struct SceneTextureData final
    {
        std::string logicalPath;

        core::images::RgbaImage image;

        bool hasTransparentPixels =
            false;

        bool hasZeroAlphaPixels =
            false;

        bool hasPartialAlphaPixels =
            false;
    };

    struct SceneModelMaterial final
    {
        std::int32_t diffuseTextureIndex =
            -1;

        SceneAlphaMode alphaMode =
            SceneAlphaMode::Opaque;

        float alphaCutoff =
            0.5f;
    };

    struct SceneTerrainLayer final
    {
        std::size_t textureIndex = 0;

        std::array<float, 4>
            uProjection{};

        std::array<float, 4>
            vProjection{};
    };

    struct SceneTerrainPass final
    {
        std::uint32_t layerCount = 0;

        std::array<
            SceneTerrainLayer,
            4>
            layers{};

        core::images::RgbaImage
            blendMap;
    };

    struct SceneTerrainMaterial final
    {
        std::vector<
            SceneTerrainPass>
            passes;
    };

    struct SceneMesh final
    {
        core::assets::MeshData geometry;

        std::vector<SceneModelMaterial>
            modelMaterials;

        std::int32_t terrainMaterialIndex =
            -1;
    };

    struct SceneInstance final
    {
        std::size_t meshIndex = 0;

        core::math::Transform3x4 transform;
    };

    struct SceneRenderData final
    {
        std::vector<SceneMesh>
            meshes;

        std::vector<SceneInstance>
            instances;

        std::vector<SceneTextureData>
            textures;

        std::vector<SceneTerrainMaterial>
            terrainMaterials;
    };
}