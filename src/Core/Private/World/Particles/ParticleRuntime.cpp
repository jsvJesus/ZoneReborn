#include "Core/World/Particles/ParticleRuntime.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <variant>

namespace
{
    constexpr float Pi =
        3.14159265358979323846f;

    constexpr float TwoPi =
        Pi *
        2.0f;

    core::math::Vector3 Add(
        const core::math::Vector3& first,
        const core::math::Vector3& second) noexcept
    {
        return
        {
            first.x + second.x,
            first.y + second.y,
            first.z + second.z
        };
    }

    core::math::Vector3 Subtract(
        const core::math::Vector3& first,
        const core::math::Vector3& second) noexcept
    {
        return
        {
            first.x - second.x,
            first.y - second.y,
            first.z - second.z
        };
    }

    core::math::Vector3 Scale(
        const core::math::Vector3& value,
        const float scale) noexcept
    {
        return
        {
            value.x * scale,
            value.y * scale,
            value.z * scale
        };
    }

    float LengthSquared(
        const core::math::Vector3& value) noexcept
    {
        return
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;
    }

    float Length(
        const core::math::Vector3& value) noexcept
    {
        return
            std::sqrt(
                LengthSquared(
                    value));
    }

    float Dot(
        const core::math::Vector3& first,
        const core::math::Vector3& second) noexcept
    {
        return
            first.x * second.x +
            first.y * second.y +
            first.z * second.z;
    }

    float MaxTransformScale(
        const core::math::Transform3x4& transform) noexcept
    {
        const float scaleX =
            std::sqrt(
                transform.values[0] *
                    transform.values[0] +
                transform.values[1] *
                    transform.values[1] +
                transform.values[2] *
                    transform.values[2]);

        const float scaleY =
            std::sqrt(
                transform.values[3] *
                    transform.values[3] +
                transform.values[4] *
                    transform.values[4] +
                transform.values[5] *
                    transform.values[5]);

        const float scaleZ =
            std::sqrt(
                transform.values[6] *
                    transform.values[6] +
                transform.values[7] *
                    transform.values[7] +
                transform.values[8] *
                    transform.values[8]);

        return
            std::max(
                {
                    scaleX,
                    scaleY,
                    scaleZ,
                    0.000001f
                });
    }

    core::math::Vector3 Normalize(
        const core::math::Vector3& value) noexcept
    {
        const float length =
            Length(
                value);

        if (length <=
            0.000001f)
        {
            return {};
        }

        return
            Scale(
                value,
                1.0f /
                    length);
    }

    float Lerp(
        const float first,
        const float second,
        const float factor) noexcept
    {
        return
            first +
            (
                second -
                first
            ) *
            factor;
    }

    core::math::Vector3 TransformPoint(
        const core::math::Vector3& point,
        const core::math::Transform3x4& transform) noexcept
    {
        return
        {
            point.x * transform.values[0] +
                point.y * transform.values[3] +
                point.z * transform.values[6] +
                transform.values[9],

            point.x * transform.values[1] +
                point.y * transform.values[4] +
                point.z * transform.values[7] +
                transform.values[10],

            point.x * transform.values[2] +
                point.y * transform.values[5] +
                point.z * transform.values[8] +
                transform.values[11]
        };
    }

    core::math::Vector3 TransformDirection(
        const core::math::Vector3& direction,
        const core::math::Transform3x4& transform) noexcept
    {
        return
        {
            direction.x * transform.values[0] +
                direction.y * transform.values[3] +
                direction.z * transform.values[6],

            direction.x * transform.values[1] +
                direction.y * transform.values[4] +
                direction.z * transform.values[7],

            direction.x * transform.values[2] +
                direction.y * transform.values[5] +
                direction.z * transform.values[8]
        };
    }

    bool IsActionActive(
        const core::world::particles::ParticleActionCommon& common,
        const float systemAge,
        const float particleAge) noexcept
    {
        if (systemAge <
            common.delay)
        {
            return false;
        }

        return
            particleAge >=
            common.minimumAge;
    }

    void EvaluateTint(
        const core::world::particles::ParticleTintShaderAction& action,
        const float particleAge,
        std::array<float, 4>& output) noexcept
    {
        if (action.tints.empty())
        {
            return;
        }

        float time =
            particleAge;

        if (action.repeat &&
            action.period >
                0.000001f)
        {
            time =
                std::fmod(
                    time,
                    action.period);

            if (time <
                0.0f)
            {
                time +=
                    action.period;
            }
        }

        if (time <=
            action.tints.front().time)
        {
            output =
                action.tints.front().colour;

            return;
        }

        if (time >=
            action.tints.back().time)
        {
            output =
                action.tints.back().colour;

            return;
        }

        for (std::size_t index = 1;
             index <
                action.tints.size();
             ++index)
        {
            const auto& right =
                action.tints[
                    index];

            if (time >
                right.time)
            {
                continue;
            }

            const auto& left =
                action.tints[
                    index - 1];

            const float duration =
                right.time -
                left.time;

            float factor =
                0.0f;

            if (duration >
                0.000001f)
            {
                factor =
                    std::clamp(
                        (
                            time -
                            left.time
                        ) /
                        duration,
                        0.0f,
                        1.0f);
            }

            for (std::size_t colourIndex = 0;
                 colourIndex < 4;
                 ++colourIndex)
            {
                output[
                    colourIndex] =
                    Lerp(
                        left.colour[
                            colourIndex],
                        right.colour[
                            colourIndex],
                        factor);
            }

            return;
        }
    }
}

namespace core::world::particles
{
    bool ParticleRuntimeSystem::Initialize(
        const ParticleSystemDefinition& definition,
        const math::Transform3x4& transform,
        const std::uint32_t randomSeed,
        std::string& error)
    {
        error.clear();

        definition_ =
            definition;

        transform_ =
            transform;

        particles_.clear();

        statistics_ =
            {};

        age_ =
            0.0f;

        capacity_ =
            definition_.capacity >
                0
                ? static_cast<std::size_t>(
                    definition_.capacity)
                : 0;

        particles_.reserve(
            capacity_);

        sourceAccumulators_.assign(
            definition_.actions.size(),
            0.0f);

        randomState_ =
            randomSeed !=
                0
                ? randomSeed
                : 0x12345678u;

        initialized_ =
            true;

        return true;
    }

    float ParticleRuntimeSystem::Random01() noexcept
    {
        randomState_ ^=
            randomState_ <<
            13u;

        randomState_ ^=
            randomState_ >>
            17u;

        randomState_ ^=
            randomState_ <<
            5u;

        return
            static_cast<float>(
                randomState_ &
                0x00FFFFFFu) /
            static_cast<float>(
                0x01000000u);
    }

    math::Vector3 ParticleRuntimeSystem::SampleVector(
        const ParticleVectorGenerator& generator) noexcept
    {
        switch (generator.type)
        {
            case ParticleVectorGeneratorType::Point:
            {
                return
                    generator.position;
            }

            case ParticleVectorGeneratorType::Line:
            {
                return
                    Add(
                        generator.origin,
                        Scale(
                            generator.direction,
                            Random01()));
            }

            case ParticleVectorGeneratorType::Box:
            {
                return
                {
                    Lerp(
                        generator.corner.x,
                        generator.opposite.x,
                        Random01()),

                    Lerp(
                        generator.corner.y,
                        generator.opposite.y,
                        Random01()),

                    Lerp(
                        generator.corner.z,
                        generator.opposite.z,
                        Random01())
                };
            }

            case ParticleVectorGeneratorType::Sphere:
            {
                const float z =
                    Random01() *
                        2.0f -
                    1.0f;

                const float angle =
                    Random01() *
                    TwoPi;

                const float radial =
                    std::sqrt(
                        std::max(
                            0.0f,
                            1.0f -
                                z * z));

                const math::Vector3 direction
                {
                    radial *
                        std::cos(
                            angle),

                    z,

                    radial *
                        std::sin(
                            angle)
                };

                const float minimumRadius =
                    std::max(
                        generator.minRadius,
                        0.0f);

                const float maximumRadius =
                    std::max(
                        generator.maxRadius,
                        minimumRadius);

                const float minimumCubed =
                    minimumRadius *
                    minimumRadius *
                    minimumRadius;

                const float maximumCubed =
                    maximumRadius *
                    maximumRadius *
                    maximumRadius;

                const float radius =
                    std::cbrt(
                        Lerp(
                            minimumCubed,
                            maximumCubed,
                            Random01()));

                return
                    Add(
                        generator.centre,
                        Scale(
                            direction,
                            radius));
            }

            case ParticleVectorGeneratorType::Cylinder:
            {
                const float minimumRadius =
                    std::max(
                        generator.minRadius,
                        0.0f);

                const float maximumRadius =
                    std::max(
                        generator.maxRadius,
                        minimumRadius);

                const float radiusSquared =
                    Lerp(
                        minimumRadius *
                            minimumRadius,
                        maximumRadius *
                            maximumRadius,
                        Random01());

                const float radius =
                    std::sqrt(
                        std::max(
                            radiusSquared,
                            0.0f));

                const float angle =
                    Random01() *
                    TwoPi;

                math::Vector3 basisU =
                    generator.basisU;

                math::Vector3 basisV =
                    generator.basisV;

                if (LengthSquared(
                        basisU) <=
                    0.000001f)
                {
                    basisU =
                    {
                        1.0f,
                        0.0f,
                        0.0f
                    };
                }

                if (LengthSquared(
                        basisV) <=
                    0.000001f)
                {
                    basisV =
                    {
                        0.0f,
                        0.0f,
                        1.0f
                    };
                }

                const math::Vector3 radialOffset =
                    Add(
                        Scale(
                            basisU,
                            std::cos(
                                angle) *
                                radius),

                        Scale(
                            basisV,
                            std::sin(
                                angle) *
                                radius));

                const math::Vector3 axialOffset =
                    Scale(
                        generator.direction,
                        Random01());

                return
                    Add(
                        generator.origin,
                        Add(
                            radialOffset,
                            axialOffset));
            }

            case ParticleVectorGeneratorType::None:
            case ParticleVectorGeneratorType::Unsupported:
            default:
            {
                return {};
            }
        }
    }

    bool ParticleRuntimeSystem::IsSourceActive(
        const ParticleSourceAction& source) const noexcept
    {
        if (!source.timeTriggered)
        {
            return false;
        }

        const float sourceAge =
            age_ -
            source.common.delay;

        if (sourceAge <
            0.0f)
        {
            return false;
        }

        if (source.allowedTimeInSeconds >
                0.0f &&
            sourceAge >
                source.allowedTimeInSeconds)
        {
            return false;
        }

        if (source.activePeriod >
                0.0f &&
            source.sleepPeriod >=
                0.0f)
        {
            const float cycleLength =
                source.activePeriod +
                source.sleepPeriod;

            if (cycleLength >
                0.000001f)
            {
                const float phase =
                    std::fmod(
                        sourceAge,
                        cycleLength);

                return
                    phase <=
                    source.activePeriod;
            }
        }

        return true;
    }

    void ParticleRuntimeSystem::SpawnFromSource(
        const ParticleSourceAction& source,
        const float deltaSeconds,
        float& accumulator) noexcept
    {
        if (capacity_ ==
                0 ||
            source.rate <=
                0.0f ||
            !IsSourceActive(
                source))
        {
            return;
        }

        accumulator +=
            source.rate *
            deltaSeconds;

        const std::size_t requested =
            static_cast<std::size_t>(
                std::floor(
                    accumulator));

        if (requested ==
            0)
        {
            return;
        }

        accumulator -=
            static_cast<float>(
                requested);

        const std::size_t available =
            capacity_ >
                particles_.size()
                ? capacity_ -
                    particles_.size()
                : 0;

        const std::size_t spawnCount =
            std::min(
                requested,
                available);

        for (std::size_t index = 0;
             index < spawnCount;
             ++index)
        {
            SpawnParticle(
                source);
        }
    }

    void ParticleRuntimeSystem::SpawnParticle(
        const ParticleSourceAction& source) noexcept
    {
        if (particles_.size() >=
            capacity_)
        {
            return;
        }

        ParticleRuntimeParticle
            particle;

        math::Vector3 localPosition =
            SampleVector(
                source.positionSource);

        localPosition =
            Add(
                localPosition,
                definition_.localOffset);

        if (definition_.explicitTransform)
        {
            localPosition =
                Add(
                    localPosition,
                    definition_.explicitPosition);
        }

        particle.position =
            TransformPoint(
                localPosition,
                transform_);

        particle.previousPosition =
            particle.position;

        const math::Vector3 localVelocity =
            SampleVector(
                source.velocitySource);

        particle.velocity =
            TransformDirection(
                localVelocity,
                transform_);

        if (source.maxSpeed >
            0.0f)
        {
            const float speed =
                Length(
                    particle.velocity);

            if (speed >
                    source.maxSpeed &&
                speed >
                    0.000001f)
            {
                particle.velocity =
                    Scale(
                        particle.velocity,
                        source.maxSpeed /
                            speed);
            }
        }

        const float minimumSize =
            std::min(
                source.minimumSize,
                source.maximumSize);

        const float maximumSize =
            std::max(
                source.minimumSize,
                source.maximumSize);

        float size =
            Lerp(
                minimumSize,
                maximumSize,
                Random01());

        if (size <=
            0.0f)
        {
            size =
                1.0f;
        }

        particle.initialSize =
            size;

        particle.size =
            size;

        particle.colour =
            source.initialColour;

        particle.rotation =
            Lerp(
                source.initialRotation[0],
                source.initialRotation[1],
                Random01());

        particle.rotation +=
            Lerp(
                source.randomInitialRotation[0],
                source.randomInitialRotation[1],
                Random01());

        if (source.randomSpin)
        {
            particle.angularVelocity =
                Lerp(
                    source.minSpin,
                    source.maxSpin,
                    Random01());
        }
        else
        {
            particle.angularVelocity =
                source.minSpin;
        }

        particles_.push_back(
            std::move(
                particle));

        ++statistics_.spawned;
    }

    bool ParticleRuntimeSystem::ApplyBarrier(
        ParticleRuntimeParticle& particle,
        const ParticleBarrierAction& action) noexcept
    {
        if (action.shape !=
            3)
        {
            return true;
        }

        const float radius =
            std::abs(
                action.radius) *
            MaxTransformScale(
                transform_);

        if (radius <=
            0.000001f)
        {
            return true;
        }

        const math::Vector3 localCentre =
            Add(
                definition_.localOffset,
                action.vectorA);

        const math::Vector3 centre =
            TransformPoint(
                localCentre,
                transform_);

        const math::Vector3 previousDelta =
            Subtract(
                particle.previousPosition,
                centre);

        const math::Vector3 currentDelta =
            Subtract(
                particle.position,
                centre);

        const float previousDistance =
            Length(
                previousDelta);

        const float currentDistance =
            Length(
                currentDelta);

        const bool previousInside =
            previousDistance <=
            radius;

        const bool currentInside =
            currentDistance <=
            radius;

        if (previousInside ==
            currentInside)
        {
            return true;
        }

        math::Vector3 normal =
            Normalize(
                currentDelta);

        if (LengthSquared(
                normal) <=
            0.000001f)
        {
            normal =
                Normalize(
                    previousDelta);
        }

        if (LengthSquared(
                normal) <=
            0.000001f)
        {
            return true;
        }

        ++statistics_.barrierInteractions;

        switch (action.reaction)
        {
            case 0:
            {
                constexpr float SurfaceEpsilon =
                    0.001f;

                const float targetRadius =
                    previousInside
                        ? std::max(
                            radius -
                                SurfaceEpsilon,
                            0.0f)
                        : radius +
                            SurfaceEpsilon;

                particle.position =
                    Add(
                        centre,
                        Scale(
                            normal,
                            targetRadius));

                const float normalVelocity =
                    Dot(
                        particle.velocity,
                        normal);

                const bool movingOut =
                    previousInside &&
                    normalVelocity >
                        0.0f;

                const bool movingIn =
                    !previousInside &&
                    normalVelocity <
                        0.0f;

                if (movingOut ||
                    movingIn)
                {
                    particle.velocity =
                        Subtract(
                            particle.velocity,
                            Scale(
                                normal,
                                normalVelocity *
                                    2.0f));
                }

                return true;
            }
        
            case 1:
            {
                return false;
            }
        
            case 3:
            {
                const float overshoot =
                    std::abs(
                        currentDistance -
                        radius);

                float targetRadius =
                    radius;

                if (previousInside)
                {
                    targetRadius =
                        std::max(
                            radius -
                                overshoot,
                            0.0f);
                }
                else
                {
                    targetRadius =
                        radius +
                        overshoot;
                }

                particle.position =
                    Add(
                        centre,
                        Scale(
                            normal,
                            -targetRadius));

                return true;
            }

            default:
            {
                return true;
            }
        }
    }

    bool ParticleRuntimeSystem::ApplyCollide(
        ParticleRuntimeParticle& particle,
        const ParticleCollideAction& action) noexcept
    {
        const math::Vector3 localPlanePoint =
            Add(
                definition_.localOffset,
                {
                    0.0f,
                    action.yOffset,
                    0.0f
                });

        const math::Vector3 planePoint =
            TransformPoint(
                localPlanePoint,
                transform_);

        math::Vector3 planeNormal =
            TransformDirection(
                {
                    0.0f,
                    1.0f,
                    0.0f
                },
                transform_);

        planeNormal =
            Normalize(
                planeNormal);

        if (LengthSquared(
                planeNormal) <=
            0.000001f)
        {
            return true;
        }

        const float previousDistance =
            Dot(
                Subtract(
                    particle.previousPosition,
                    planePoint),
                planeNormal);

        const float currentDistance =
            Dot(
                Subtract(
                    particle.position,
                    planePoint),
                planeNormal);
        
        if (previousDistance <
                0.0f ||
            currentDistance >=
                0.0f)
        {
            return true;
        }

        const float normalVelocity =
            Dot(
                particle.velocity,
                planeNormal);

        if (normalVelocity >=
            0.0f)
        {
            return true;
        }

        ++statistics_.collisionInteractions;
        
        particle.position =
            Subtract(
                particle.position,
                Scale(
                    planeNormal,
                    currentDistance));

        const math::Vector3 normalComponent =
            Scale(
                planeNormal,
                normalVelocity);

        const math::Vector3 tangentComponent =
            Subtract(
                particle.velocity,
                normalComponent);

        const float elasticity =
            std::max(
                action.elasticity,
                0.0f);

        const float friction =
            std::clamp(
                action.frictionCoefficient,
                0.0f,
                1.0f);

        particle.velocity =
            Add(
                Scale(
                    tangentComponent,
                    1.0f -
                        friction),

                Scale(
                    planeNormal,
                    -normalVelocity *
                        elasticity));

        if (action.minAddedRotation !=
                0.0f ||
            action.maxAddedRotation !=
                0.0f)
        {
            const float minimumRotation =
                std::min(
                    action.minAddedRotation,
                    action.maxAddedRotation);

            const float maximumRotation =
                std::max(
                    action.minAddedRotation,
                    action.maxAddedRotation);

            particle.angularVelocity +=
                Lerp(
                    minimumRotation,
                    maximumRotation,
                    Random01());
        }

        return true;
    }

    bool ParticleRuntimeSystem::UpdateParticle(
        ParticleRuntimeParticle& particle,
        const float deltaSeconds) noexcept
    {
        particle.previousPosition =
            particle.position;

        particle.age +=
            deltaSeconds;

        particle.rotation +=
            particle.angularVelocity *
            deltaSeconds;

        for (const ParticleActionDefinition& actionDefinition :
             definition_.actions)
        {
            switch (actionDefinition.type)
            {
                case ParticleActionType::Source:
                {
                    break;
                }

                case ParticleActionType::Sink:
                {
                    const ParticleSinkAction* action =
                        std::get_if<ParticleSinkAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    if (action->maximumAge >
                            0.0f &&
                        particle.age >=
                            action->maximumAge)
                    {
                        return false;
                    }

                    if (action->minimumSpeed >=
                        0.0f)
                    {
                        const float speed =
                            Length(
                                particle.velocity);

                        if (speed <
                            action->minimumSpeed)
                        {
                            return false;
                        }
                    }

                    break;
                }

                case ParticleActionType::Force:
                {
                    const ParticleForceAction* action =
                        std::get_if<ParticleForceAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    const math::Vector3 force =
                        TransformDirection(
                            action->vector,
                            transform_);

                    particle.velocity =
                        Add(
                            particle.velocity,
                            Scale(
                                force,
                                deltaSeconds));

                    break;
                }

                case ParticleActionType::Stream:
                {
                    const ParticleStreamAction* action =
                        std::get_if<ParticleStreamAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    const math::Vector3 targetVelocity =
                        TransformDirection(
                            action->vector,
                            transform_);

                    if (action->halfLife >
                        0.000001f)
                    {
                        const float factor =
                            1.0f -
                            std::exp(
                                -0.6931471805599453f *
                                deltaSeconds /
                                action->halfLife);

                        particle.velocity.x =
                            Lerp(
                                particle.velocity.x,
                                targetVelocity.x,
                                factor);

                        particle.velocity.y =
                            Lerp(
                                particle.velocity.y,
                                targetVelocity.y,
                                factor);

                        particle.velocity.z =
                            Lerp(
                                particle.velocity.z,
                                targetVelocity.z,
                                factor);
                    }
                    else
                    {
                        particle.velocity =
                            targetVelocity;
                    }

                    break;
                }

                case ParticleActionType::Jitter:
                {
                    const ParticleJitterAction* action =
                        std::get_if<ParticleJitterAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    if (action->affectPosition)
                    {
                        const math::Vector3 jitter =
                            TransformDirection(
                                SampleVector(
                                    action->positionSource),
                                transform_);

                        particle.position =
                            Add(
                                particle.position,
                                Scale(
                                    jitter,
                                    deltaSeconds));
                    }

                    if (action->affectVelocity)
                    {
                        const math::Vector3 jitter =
                            TransformDirection(
                                SampleVector(
                                    action->velocitySource),
                                transform_);

                        particle.velocity =
                            Add(
                                particle.velocity,
                                Scale(
                                    jitter,
                                    deltaSeconds));
                    }

                    break;
                }

                case ParticleActionType::Orbitor:
                {
                    const ParticleOrbitorAction* action =
                        std::get_if<ParticleOrbitorAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    math::Vector3 localCentre =
                        Add(
                            action->point,
                            definition_.localOffset);

                    const math::Vector3 centre =
                        TransformPoint(
                            localCentre,
                            transform_);

                    math::Vector3 relative =
                        Subtract(
                            particle.position,
                            centre);

                    const float angle =
                        action->angularVelocity *
                        deltaSeconds;

                    const float cosine =
                        std::cos(
                            angle);

                    const float sine =
                        std::sin(
                            angle);

                    const float rotatedX =
                        relative.x *
                            cosine -
                        relative.z *
                            sine;

                    const float rotatedZ =
                        relative.x *
                            sine +
                        relative.z *
                            cosine;

                    relative.x =
                        rotatedX;

                    relative.z =
                        rotatedZ;

                    particle.position =
                        Add(
                            centre,
                            relative);

                    if (action->affectVelocity)
                    {
                        const float velocityX =
                            particle.velocity.x *
                                cosine -
                            particle.velocity.z *
                                sine;

                        const float velocityZ =
                            particle.velocity.x *
                                sine +
                            particle.velocity.z *
                                cosine;

                        particle.velocity.x =
                            velocityX;

                        particle.velocity.z =
                            velocityZ;
                    }

                    break;
                }

                case ParticleActionType::Magnet:
                {
                    const ParticleMagnetAction* action =
                        std::get_if<ParticleMagnetAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    const math::Vector3 centre =
                        TransformPoint(
                            definition_.localOffset,
                            transform_);

                    const math::Vector3 delta =
                        Subtract(
                            centre,
                            particle.position);

                    const float distance =
                        Length(
                            delta);

                    if (distance <=
                        0.000001f)
                    {
                        break;
                    }

                    if (action->minDistance >
                            0.0f &&
                        distance <
                            action->minDistance)
                    {
                        break;
                    }

                    const math::Vector3 direction =
                        Normalize(
                            delta);

                    particle.velocity =
                        Add(
                            particle.velocity,
                            Scale(
                                direction,
                                action->strength *
                                    deltaSeconds));

                    break;
                }

                case ParticleActionType::Scaler:
                {
                    const ParticleScalerAction* action =
                        std::get_if<ParticleScalerAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    const float scale =
                        std::max(
                            0.0f,
                            action->size +
                                action->rate *
                                particle.age);

                    particle.size =
                        particle.initialSize *
                        scale;

                    break;
                }

                case ParticleActionType::TintShader:
                {
                    const ParticleTintShaderAction* action =
                        std::get_if<ParticleTintShaderAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    EvaluateTint(
                        *action,
                        particle.age,
                        particle.colour);

                    break;
                }

                case ParticleActionType::Barrier:
                case ParticleActionType::Flare:
                case ParticleActionType::Collide:
                case ParticleActionType::Unsupported:
                default:
                {
                    break;
                }
            }
        }

        particle.position =
            Add(
                particle.position,
                Scale(
                    particle.velocity,
                    deltaSeconds));
        
        for (const ParticleActionDefinition& actionDefinition :
             definition_.actions)
        {
            switch (actionDefinition.type)
            {
                case ParticleActionType::Barrier:
                {
                    const ParticleBarrierAction* action =
                        std::get_if<ParticleBarrierAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    if (!ApplyBarrier(
                            particle,
                            *action))
                    {
                        return false;
                    }

                    break;
                }

                case ParticleActionType::Collide:
                {
                    const ParticleCollideAction* action =
                        std::get_if<ParticleCollideAction>(
                            &actionDefinition.data);

                    if (action == nullptr ||
                        !IsActionActive(
                            action->common,
                            age_,
                            particle.age))
                    {
                        break;
                    }

                    if (!ApplyCollide(
                            particle,
                            *action))
                    {
                        return false;
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }
        }

        return true;
    }

    void ParticleRuntimeSystem::Update(
        float deltaSeconds) noexcept
    {
        if (!initialized_ ||
            deltaSeconds <=
                0.0f)
        {
            return;
        }

        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.1f);

        age_ +=
            deltaSeconds;

        for (std::size_t actionIndex = 0;
             actionIndex <
                definition_.actions.size();
             ++actionIndex)
        {
            const ParticleActionDefinition& definition =
                definition_.actions[
                    actionIndex];

            if (definition.type !=
                ParticleActionType::Source)
            {
                continue;
            }

            const ParticleSourceAction* source =
                std::get_if<ParticleSourceAction>(
                    &definition.data);

            if (source == nullptr)
            {
                continue;
            }

            SpawnFromSource(
                *source,
                deltaSeconds,
                sourceAccumulators_[
                    actionIndex]);
        }

        std::size_t index =
            0;

        while (index <
               particles_.size())
        {
            if (UpdateParticle(
                    particles_[
                        index],
                    deltaSeconds))
            {
                ++index;

                continue;
            }

            ++statistics_.killed;

            if (index !=
                particles_.size() -
                    1)
            {
                particles_[
                    index] =
                    std::move(
                        particles_.back());
            }

            particles_.pop_back();
        }
    }

    const ParticleSystemDefinition&
    ParticleRuntimeSystem::Definition() const noexcept
    {
        return
            definition_;
    }

    const math::Transform3x4&
    ParticleRuntimeSystem::Transform() const noexcept
    {
        return
            transform_;
    }

    const std::vector<ParticleRuntimeParticle>&
    ParticleRuntimeSystem::Particles() const noexcept
    {
        return
            particles_;
    }

    std::size_t
    ParticleRuntimeSystem::ActiveParticleCount() const noexcept
    {
        return
            particles_.size();
    }

    std::size_t
    ParticleRuntimeSystem::Capacity() const noexcept
    {
        return
            capacity_;
    }

    float ParticleRuntimeSystem::Age() const noexcept
    {
        return
            age_;
    }

    const ParticleRuntimeStatistics&
    ParticleRuntimeSystem::Statistics() const noexcept
    {
        return
            statistics_;
    }
}