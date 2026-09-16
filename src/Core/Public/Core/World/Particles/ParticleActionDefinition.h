#pragma once

#include "Core/Math/Vector3.h"
#include "Core/World/Particles/ParticleVectorGenerator.h"

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace core::world::particles
{
    enum class ParticleActionType : std::uint8_t
    {
        Unsupported = 0,
        Source,
        Sink,
        TintShader,
        Orbitor,
        Jitter,
        Stream,
        Force,
        Magnet,
        Barrier,
        Scaler,
        Flare,
        Collide
    };

    struct ParticleActionCommon final
    {
        std::string name;

        float delay =
            0.0f;

        float minimumAge =
            0.0f;
    };

    struct ParticleSourceAction final
    {
        ParticleActionCommon common;

        ParticleVectorGenerator
            positionSource;

        ParticleVectorGenerator
            velocitySource;

        bool motionTriggered =
            false;

        bool timeTriggered =
            false;

        bool grounded =
            false;

        float dropDistance =
            0.0f;

        float rate =
            0.0f;

        float sensitivity =
            0.0f;

        float maxSpeed =
            0.0f;

        float activePeriod =
            0.0f;

        float sleepPeriod =
            0.0f;

        float sleepPeriodMax =
            0.0f;

        float minimumSize =
            0.0f;

        float maximumSize =
            0.0f;

        std::int32_t forcedUnitSize =
            0;

        float allowedTimeInSeconds =
            0.0f;

        std::array<float, 2>
            initialRotation{};

        std::array<float, 2>
            randomInitialRotation{};

        std::array<float, 4>
            initialColour
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        bool randomSpin =
            false;

        float minSpin =
            0.0f;

        float maxSpin =
            0.0f;

        bool ignoreRotation =
            false;

        float inheritVelocity =
            0.0f;
    };

    struct ParticleSinkAction final
    {
        ParticleActionCommon common;

        float maximumAge =
            0.0f;

        float minimumSpeed =
            -1.0f;

        bool outsideOnly =
            false;
    };

    struct ParticleTintKey final
    {
        float time =
            0.0f;

        std::array<float, 4>
            colour{};
    };

    struct ParticleTintShaderAction final
    {
        ParticleActionCommon common;

        bool repeat =
            false;

        float period =
            0.0f;

        float fogAmount =
            0.0f;

        std::vector<ParticleTintKey>
            tints;
    };

    struct ParticleOrbitorAction final
    {
        ParticleActionCommon common;

        math::Vector3 point{};

        float angularVelocity =
            0.0f;

        bool affectVelocity =
            false;
    };

    struct ParticleJitterAction final
    {
        ParticleActionCommon common;

        bool affectPosition =
            false;

        bool affectVelocity =
            false;

        ParticleVectorGenerator
            positionSource;

        ParticleVectorGenerator
            velocitySource;
    };

    struct ParticleStreamAction final
    {
        ParticleActionCommon common;

        math::Vector3 vector{};

        float halfLife =
            0.0f;
    };

    struct ParticleForceAction final
    {
        ParticleActionCommon common;

        math::Vector3 vector{};
    };

    struct ParticleMagnetAction final
    {
        ParticleActionCommon common;

        float strength =
            0.0f;

        float minDistance =
            0.0f;
    };

    struct ParticleBarrierAction final
    {
        ParticleActionCommon common;

        std::int32_t shape =
            0;

        std::int32_t reaction =
            0;

        math::Vector3 vectorA{};
        math::Vector3 vectorB{};

        float radius =
            0.0f;
    };

    struct ParticleScalerAction final
    {
        ParticleActionCommon common;

        float size =
            1.0f;

        float rate =
            0.0f;
    };

    struct ParticleFlareAction final
    {
        ParticleActionCommon common;

        std::string flareName;

        std::int32_t flareStep =
            0;

        bool colourize =
            false;

        bool useParticleSize =
            false;

        bool hasDirection =
            false;

        math::Vector3 direction
        {
            1.0f,
            0.0f,
            0.0f
        };

        float visibilityDotMin =
            -1.0f;

        float visibilityDotMax =
            1.0f;

        float visibilityMinValue =
            1.0f;

        float visibilityMaxValue =
            1.0f;
    };

    struct ParticleCollideAction final
    {
        ParticleActionCommon common;

        bool spriteBased =
            false;

        float elasticity =
            0.0f;

        float minAddedRotation =
            0.0f;

        float maxAddedRotation =
            0.0f;

        std::int32_t entityId =
            0;

        std::string soundTag;

        bool soundEnabled =
            false;

        std::int32_t soundSourceIndex =
            0;

        std::string soundProject;
        std::string soundGroup;
        std::string soundName;

        bool cylinderCollide =
            false;

        float yOffset =
            0.0f;

        float frictionCoefficient =
            0.0f;
    };

    using ParticleActionData =
        std::variant<
            std::monostate,
            ParticleSourceAction,
            ParticleSinkAction,
            ParticleTintShaderAction,
            ParticleOrbitorAction,
            ParticleJitterAction,
            ParticleStreamAction,
            ParticleForceAction,
            ParticleMagnetAction,
            ParticleBarrierAction,
            ParticleScalerAction,
            ParticleFlareAction,
            ParticleCollideAction>;

    struct ParticleActionDefinition final
    {
        ParticleActionType type =
            ParticleActionType::Unsupported;

        std::string typeName;

        ParticleActionData
            data;
    };
}