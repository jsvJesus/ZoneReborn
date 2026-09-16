#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Images/RgbaImage.h"
#include "Core/Math/Transform3x4.h"

#include "Core/Animation/ScalarAnimation.h"
#include "Core/World/Sky/SkyDefinition.h"
#include "Core/World/Particles/ParticleDefinition.h"

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

        bool generateMipmaps =
            true;
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
        std::size_t textureIndex =
            0;

        std::array<float, 4>
            uProjection{};

        std::array<float, 4>
            vProjection{};
    };

    struct SceneTerrainPass final
    {
        std::uint32_t layerCount =
            0;

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

    struct SceneWaterMaterial final
    {
        std::int32_t waveTextureIndex =
            -1;

        std::int32_t foamTextureIndex =
            -1;

        std::array<float, 4> deepColour{};
        std::array<float, 4> reflectionTint{};
        std::array<float, 4> refractionTint{};

        std::array<float, 2> waveScale{};
        std::array<float, 2> scrollSpeed1{};
        std::array<float, 2> scrollSpeed2{};

        float reflectionStrength =
            0.0f;

        float refractionStrength =
            0.0f;

        float fresnelConstant =
            0.0f;

        float fresnelExponent =
            0.0f;

        float windVelocity =
            0.0f;

        float textureTessellation =
            1.0f;

        float foamIntersection =
            0.0f;

        float foamMultiplier =
            0.0f;

        float foamTiling =
            0.0f;

        float depth =
            1.0f;

        float fadeDepth =
            0.0f;

        float smoothness =
            0.0f;

        float sunPower =
            1.0f;

        float sunScale =
            0.0f;

        bool useEdgeAlpha =
            false;

        bool useSimulation =
            false;
    };

    struct SceneOmniLight final
    {
        std::string guid;

        std::array<float, 3>
            position{};

        std::array<float, 3>
            colour{};

        float innerRadius =
            0.0f;

        float outerRadius =
            0.0f;

        float multiplier =
            1.0f;

        std::int32_t priority =
            0;

        std::int32_t lightType =
            0;

        bool isDynamic =
            false;

        bool isStatic =
            false;

        bool specular =
            false;
    };

    struct SceneSpotLight final
    {
        std::string guid;

        std::array<float, 3>
            position{};

        std::array<float, 3>
            direction{};

        std::array<float, 3>
            colour{};

        float innerRadius =
            0.0f;

        float outerRadius =
            0.0f;

        float cosConeAngle =
            1.0f;

        float multiplier =
            1.0f;

        std::int32_t priority =
            0;

        std::int32_t lightType =
            0;

        bool isDynamic =
            false;

        bool isStatic =
            false;

        bool specular =
            false;
    };

    struct ScenePulseLightFrame final
    {
        float time =
            0.0f;

        float value =
            1.0f;
    };

    struct ScenePulseLight final
    {
        std::string guid;

        std::array<float, 3>
            position{};

        std::array<float, 3>
            colour{};

        float innerRadius =
            0.0f;

        float outerRadius =
            0.0f;

        float multiplier =
            1.0f;

        std::int32_t priority =
            0;

        core::animation::ScalarAnimationTrack
            animation;
    };

    struct SceneFlare final
    {
        std::string guid;
        std::string resource;

        std::array<float, 3>
            position{};

        std::array<float, 4>
            colour
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        std::size_t textureIndex =
            0;

        float size =
            1.0f;

        float depth =
            1.0f;

        float maxDistance =
            0.0f;

        float area =
            1.0f;

        float fadeSpeed =
            1.0f;
    };

    struct SceneMesh final
    {
        core::assets::MeshData geometry;

        std::vector<SceneModelMaterial>
            modelMaterials;

        std::int32_t terrainMaterialIndex =
            -1;

        std::int32_t waterMaterialIndex =
            -1;
    };

    struct SceneInstance final
    {
        std::size_t meshIndex =
            0;

        core::math::Transform3x4
            transform;

        float maximumDistance =
            0.0f;
    };

    struct SceneLodLevel final
    {
        std::vector<std::size_t>
            meshIndices;

        float maximumDistance =
            0.0f;
    };

    struct SceneLodInstance final
    {
        core::math::Transform3x4
            transform;

        std::array<
            SceneLodLevel,
            4>
            levels{};

        std::uint32_t levelCount =
            0;
    };

    struct SceneSky final
    {
        bool enabled =
            false;

        std::int32_t gradientTextureIndex =
            -1;

        core::world::sky::SkyDefinition
            definition;
    };

    struct SceneParticleEmitter final
    {
        std::string resource;

        core::world::particles::ParticleSystemDefinition
            system;

        core::math::Transform3x4
            transform;
    };

    struct SceneRenderData final
    {
        std::vector<SceneMesh>
            meshes;

        std::vector<SceneInstance>
            instances;

        std::vector<SceneLodInstance>
            lodInstances;

        std::vector<SceneTextureData>
            textures;

        std::vector<SceneTerrainMaterial>
            terrainMaterials;

        std::vector<SceneWaterMaterial>
            waterMaterials;

        std::vector<SceneOmniLight>
            omniLights;

        std::vector<SceneSpotLight>
            spotLights;

        std::vector<ScenePulseLight>
            pulseLights;

        std::vector<SceneFlare>
            flares;

        std::vector<SceneParticleEmitter>
            particleEmitters;

        SceneSky
            sky;
    };
}