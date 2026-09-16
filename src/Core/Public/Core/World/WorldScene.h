#pragma once

#include "Core/Math/Transform3x4.h"
#include "Core/Math/Vector3.h"
#include "Core/World/SpaceSettings.h"
#include "Core/World/Vlo/VloResource.h"

#include "Core/Animation/ScalarAnimation.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::world
{
    struct WorldModelInstance final
    {
        std::string chunkId;
        std::string modelReference;

        math::Transform3x4 transform;

        bool shell = false;
    };

    struct WorldSpeedTreeInstance final
    {
        std::string chunkId;

        std::string resourceReference;
        std::string sptLogicalPath;

        std::int32_t seed = 0;

        math::Transform3x4 transform;

        bool reflectionVisible = false;
    };

    struct WorldTerrainInstance final
    {
        std::string chunkId;

        std::string resourceReference;
        std::string cdataLogicalPath;

        math::Transform3x4 transform;
    };

    struct WorldLargeObjectReference final
    {
        std::string uid;
        std::string type;

        std::string vloLogicalPath;
        std::string odataLogicalPath;

        bool vloExists = false;
        bool odataExists = false;

        bool vloLoaded = false;

        vlo::VloResource
            vloResource;

        std::vector<std::string>
            chunkIds;
    };

    struct WorldParticleInstance final
    {
        std::string chunkId;

        std::string resourceReference;
        std::string particleLogicalPath;

        math::Transform3x4 transform;

        bool reflectionVisible = false;
    };

    struct WorldOmniLightInstance final
    {
        std::string chunkId;
        std::string guid;

        math::Vector3 position;
        math::Vector3 colour;

        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        float multiplier = 1.0f;

        std::int32_t priority = 0;
        std::int32_t lightType = 0;

        bool isDynamic = false;
        bool isStatic = false;
        bool specular = false;
    };

    struct WorldSpotLightInstance final
    {
        std::string chunkId;
        std::string guid;

        math::Vector3 position;
        math::Vector3 direction;
        math::Vector3 colour;

        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        float cosConeAngle = 1.0f;
        float multiplier = 1.0f;

        std::int32_t priority = 0;
        std::int32_t lightType = 0;

        bool isDynamic = false;
        bool isStatic = false;
        bool specular = false;
    };

    struct WorldPulseLightInstance final
    {
        std::string chunkId;
        std::string guid;

        math::Vector3 position;
        math::Vector3 colour;

        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        float multiplier = 1.0f;

        std::int32_t priority = 0;

        core::animation::ScalarAnimationTrack
            animation;
    };

    struct WorldFlareInstance final
    {
        std::string chunkId;
        std::string guid;
        std::string resource;

        math::Vector3 position;

        math::Vector3 colour
        {
            255.0f,
            255.0f,
            255.0f
        };

        float maxDistance =
            0.0f;

        float area =
            1.0f;

        float fadeSpeed =
            1.0f;
    };

    struct WorldScene final
    {
        std::string spaceName;

        SpaceSettings settings;

        std::vector<WorldModelInstance>
            modelInstances;

        std::vector<WorldSpeedTreeInstance>
            speedTreeInstances;

        std::vector<WorldTerrainInstance>
            terrainInstances;

        std::vector<WorldLargeObjectReference>
            largeObjects;

        std::vector<WorldParticleInstance>
            particleInstances;

        std::vector<WorldOmniLightInstance>
            omniLights;

        std::vector<WorldSpotLightInstance>
            spotLights;

        std::vector<WorldPulseLightInstance>
            pulseLights;

        std::vector<WorldFlareInstance>
            flares;

        std::size_t chunkCount = 0;
        std::size_t outdoorChunkCount = 0;
        std::size_t indoorChunkCount = 0;

        std::size_t speedTreeInstanceCount = 0;
        std::size_t terrainReferenceCount = 0;

        std::size_t particleInstanceCount = 0;
        std::size_t uniqueParticleResourceCount = 0;
        std::size_t missingParticleInstanceCount = 0;
        std::size_t missingUniqueParticleResourceCount = 0;

        std::size_t omniLightCount = 0;
        std::size_t spotLightCount = 0;
        std::size_t pulseLightCount = 0;
        std::size_t flareCount = 0;

        std::size_t largeObjectReferenceCount = 0;
        std::size_t missingLargeObjectCount = 0;
        std::size_t loadedLargeObjectCount = 0;
        std::size_t failedLargeObjectLoadCount = 0;
    };
}