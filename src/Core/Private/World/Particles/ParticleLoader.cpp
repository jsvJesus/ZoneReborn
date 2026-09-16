#include "Core/World/Particles/ParticleLoader.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using core::resources::DataSection;
    using core::world::particles::ParticleActionCommon;
    using core::world::particles::ParticleActionDefinition;
    using core::world::particles::ParticleActionType;
    using core::world::particles::ParticleBarrierAction;
    using core::world::particles::ParticleCollideAction;
    using core::world::particles::ParticleDefinition;
    using core::world::particles::ParticleFlareAction;
    using core::world::particles::ParticleForceAction;
    using core::world::particles::ParticleJitterAction;
    using core::world::particles::ParticleLoadStatistics;
    using core::world::particles::ParticleMagnetAction;
    using core::world::particles::ParticleOrbitorAction;
    using core::world::particles::ParticleRendererDefinition;
    using core::world::particles::ParticleRendererType;
    using core::world::particles::ParticleScalerAction;
    using core::world::particles::ParticleSinkAction;
    using core::world::particles::ParticleSourceAction;
    using core::world::particles::ParticleStreamAction;
    using core::world::particles::ParticleSystemDefinition;
    using core::world::particles::ParticleTextureReference;
    using core::world::particles::ParticleTintKey;
    using core::world::particles::ParticleTintShaderAction;
    using core::world::particles::ParticleVectorGenerator;
    using core::world::particles::ParticleVectorGeneratorType;

    bool ReadFloat(
        const DataSection& section,
        float& output)
    {
        return
            section.TryGetFloat(
                output);
    }

    bool ReadInteger(
        const DataSection& section,
        std::int32_t& output)
    {
        const std::int64_t* value =
            section.AsInteger();

        if (value == nullptr)
        {
            return false;
        }

        output =
            static_cast<std::int32_t>(
                *value);

        return true;
    }

    bool ReadBoolean(
        const DataSection& section,
        bool& output)
    {
        const bool* value =
            section.AsBoolean();

        if (value == nullptr)
        {
            return false;
        }

        output =
            *value;

        return true;
    }

    bool ReadString(
        const DataSection& section,
        std::string& output)
    {
        const std::string* value =
            section.AsString();

        if (value == nullptr)
        {
            return false;
        }

        output =
            *value;

        return true;
    }

    template<std::size_t Count>
    bool ReadArray(
        const DataSection& section,
        std::array<float, Count>& output)
    {
        const DataSection::FloatArray* values =
            section.AsFloats();

        if (values == nullptr ||
            values->size() != Count)
        {
            return false;
        }

        for (std::size_t index = 0;
             index < Count;
             ++index)
        {
            output[index] =
                (*values)[index];
        }

        return true;
    }

    bool ReadVector3(
        const DataSection& section,
        core::math::Vector3& output)
    {
        const DataSection::FloatArray* values =
            section.AsFloats();

        if (values == nullptr ||
            values->size() != 3)
        {
            return false;
        }

        output.x =
            (*values)[0];

        output.y =
            (*values)[1];

        output.z =
            (*values)[2];

        return true;
    }

    bool ReadOptionalFloat(
        const DataSection& section,
        const std::string_view name,
        float& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadFloat(
                *child,
                output);
    }

    bool ReadOptionalInteger(
        const DataSection& section,
        const std::string_view name,
        std::int32_t& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadInteger(
                *child,
                output);
    }

    bool ReadOptionalBoolean(
        const DataSection& section,
        const std::string_view name,
        bool& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadBoolean(
                *child,
                output);
    }

    bool ReadOptionalString(
        const DataSection& section,
        const std::string_view name,
        std::string& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadString(
                *child,
                output);
    }

    bool ReadOptionalVector3(
        const DataSection& section,
        const std::string_view name,
        core::math::Vector3& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadVector3(
                *child,
                output);
    }

    template<std::size_t Count>
    bool ReadOptionalArray(
        const DataSection& section,
        const std::string_view name,
        std::array<float, Count>& output)
    {
        const DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return true;
        }

        return
            ReadArray(
                *child,
                output);
    }

    std::string BuildResourcePath(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view reference)
    {
        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            return {};
        }

        std::filesystem::path path(
            normalized);

        if (!path.has_extension())
        {
            path.replace_extension(
                ".xml");

            normalized =
                core::resources::ResourcePath::Normalize(
                    path.generic_string());
        }

        if (normalized.starts_with(
                "res/") ||
            normalized.starts_with(
                "sys/"))
        {
            return normalized;
        }

        const std::string resPath =
            "res/" +
            normalized;

        if (resources.Exists(
                resPath))
        {
            return resPath;
        }

        const std::string sysPath =
            "sys/" +
            normalized;

        if (resources.Exists(
                sysPath))
        {
            return sysPath;
        }

        return resPath;
    }

    bool ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view reference,
        ParticleDefinition& definition,
        ParticleTextureReference& output,
        std::string& error)
    {
        output = {};
        error.clear();

        if (reference.empty())
        {
            return true;
        }

        ++definition.statistics.textureReferenceCount;

        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            error =
                "Particle texture reference is invalid.";

            return false;
        }

        output.sourceReference =
            std::string(
                reference);

        const std::filesystem::path path(
            normalized);

        const std::string extension =
            path.extension().string();

        if (extension ==
            ".texanim")
        {
            output.animated =
                true;

            ++definition.statistics
                .animatedTextureReferenceCount;

            if (normalized.starts_with(
                    "res/") ||
                normalized.starts_with(
                    "sys/"))
            {
                output.logicalPath =
                    normalized;
            }
            else
            {
                const std::string resPath =
                    "res/" +
                    normalized;

                const std::string sysPath =
                    "sys/" +
                    normalized;

                if (resources.Exists(
                        resPath))
                {
                    output.logicalPath =
                        resPath;
                }
                else if (resources.Exists(
                             sysPath))
                {
                    output.logicalPath =
                        sysPath;
                }
                else
                {
                    output.logicalPath =
                        resPath;
                }
            }

            output.exists =
                resources.Exists(
                    output.logicalPath);
        }
        else
        {
            core::assets::TextureResolver
                resolver;

            core::assets::TextureResource
                texture;

            if (!resolver.Resolve(
                    resources,
                    reference,
                    texture))
            {
                error =
                    "Unable to resolve particle texture: " +
                    std::string(
                        reference);

                return false;
            }

            output.logicalPath =
                texture.logicalPath;

            output.exists =
                texture.exists;
        }

        if (!output.exists)
        {
            ++definition.statistics
                .missingTextureCount;

            definition.missingTextures.push_back(
                output.logicalPath);
        }

        return true;
    }

    bool ReadActionCommon(
        const DataSection& section,
        ParticleActionCommon& output)
    {
        return
            ReadOptionalString(
                section,
                "name_",
                output.name) &&
            ReadOptionalFloat(
                section,
                "delay_",
                output.delay) &&
            ReadOptionalFloat(
                section,
                "minimumAge_",
                output.minimumAge);
    }

    bool ReadVectorGenerator(
        const DataSection* wrapper,
        ParticleVectorGenerator& output,
        ParticleLoadStatistics& statistics,
        std::string& error)
    {
        output = {};
        error.clear();

        if (wrapper == nullptr ||
            wrapper->children.empty())
        {
            return true;
        }

        const DataSection& section =
            wrapper->children.front();

        ++statistics.vectorGeneratorCount;

        output.typeName =
            section.name;

        if (section.name ==
            "PointVectorGenerator")
        {
            output.type =
                ParticleVectorGeneratorType::Point;
        }
        else if (section.name ==
                 "LineVectorGenerator")
        {
            output.type =
                ParticleVectorGeneratorType::Line;
        }
        else if (section.name ==
                 "CylinderVectorGenerator")
        {
            output.type =
                ParticleVectorGeneratorType::Cylinder;
        }
        else if (section.name ==
                 "SphereVectorGenerator")
        {
            output.type =
                ParticleVectorGeneratorType::Sphere;
        }
        else if (section.name ==
                 "BoxVectorGenerator")
        {
            output.type =
                ParticleVectorGeneratorType::Box;
        }
        else
        {
            output.type =
                ParticleVectorGeneratorType::Unsupported;

            ++statistics
                .unsupportedVectorGeneratorCount;

            return true;
        }

        if (!ReadOptionalString(
                section,
                "nameID_",
                output.nameId) ||
            !ReadOptionalVector3(
                section,
                "position_",
                output.position) ||
            !ReadOptionalVector3(
                section,
                "origin_",
                output.origin) ||
            !ReadOptionalVector3(
                section,
                "direction_",
                output.direction) ||
            !ReadOptionalVector3(
                section,
                "centre_",
                output.centre) ||
            !ReadOptionalVector3(
                section,
                "corner_",
                output.corner) ||
            !ReadOptionalVector3(
                section,
                "opposite_",
                output.opposite) ||
            !ReadOptionalVector3(
                section,
                "basisU_",
                output.basisU) ||
            !ReadOptionalVector3(
                section,
                "basisV_",
                output.basisV) ||
            !ReadOptionalFloat(
                section,
                "minRadius_",
                output.minRadius) ||
            !ReadOptionalFloat(
                section,
                "maxRadius_",
                output.maxRadius))
        {
            error =
                "Particle vector generator contains invalid data: " +
                section.name;

            return false;
        }

        return true;
    }

    bool ReadSourceAction(
        const DataSection& section,
        ParticleDefinition& definition,
        ParticleSourceAction& output,
        std::string& error)
    {
        if (!ReadActionCommon(
                section,
                output.common) ||
            !ReadOptionalBoolean(
                section,
                "motionTriggered_",
                output.motionTriggered) ||
            !ReadOptionalBoolean(
                section,
                "timeTriggered_",
                output.timeTriggered) ||
            !ReadOptionalBoolean(
                section,
                "grounded_",
                output.grounded) ||
            !ReadOptionalFloat(
                section,
                "dropDistance_",
                output.dropDistance) ||
            !ReadOptionalFloat(
                section,
                "rate_",
                output.rate) ||
            !ReadOptionalFloat(
                section,
                "sensitivity_",
                output.sensitivity) ||
            !ReadOptionalFloat(
                section,
                "maxSpeed_",
                output.maxSpeed) ||
            !ReadOptionalFloat(
                section,
                "activePeriod_",
                output.activePeriod) ||
            !ReadOptionalFloat(
                section,
                "sleepPeriod_",
                output.sleepPeriod) ||
            !ReadOptionalFloat(
                section,
                "sleepPeriodMax_",
                output.sleepPeriodMax) ||
            !ReadOptionalFloat(
                section,
                "minimumSize_",
                output.minimumSize) ||
            !ReadOptionalFloat(
                section,
                "maximumSize_",
                output.maximumSize) ||
            !ReadOptionalInteger(
                section,
                "forcedUnitSize_",
                output.forcedUnitSize) ||
            !ReadOptionalFloat(
                section,
                "allowedTimeInSeconds_",
                output.allowedTimeInSeconds) ||
            !ReadOptionalArray(
                section,
                "initialRotation_",
                output.initialRotation) ||
            !ReadOptionalArray(
                section,
                "randomInitialRotation_",
                output.randomInitialRotation) ||
            !ReadOptionalArray(
                section,
                "initialColour_",
                output.initialColour) ||
            !ReadOptionalBoolean(
                section,
                "randomSpin_",
                output.randomSpin) ||
            !ReadOptionalFloat(
                section,
                "minSpin_",
                output.minSpin) ||
            !ReadOptionalFloat(
                section,
                "maxSpin_",
                output.maxSpin) ||
            !ReadOptionalBoolean(
                section,
                "ignoreRotation_",
                output.ignoreRotation) ||
            !ReadOptionalFloat(
                section,
                "inheritVelocity_",
                output.inheritVelocity))
        {
            error =
                "Particle Source action contains invalid data.";

            return false;
        }

        if (!ReadVectorGenerator(
                section.FindChild(
                    "pPositionSrc"),
                output.positionSource,
                definition.statistics,
                error))
        {
            return false;
        }

        if (!ReadVectorGenerator(
                section.FindChild(
                    "pVelocitySrc"),
                output.velocitySource,
                definition.statistics,
                error))
        {
            return false;
        }

        return true;
    }

    bool ReadTintShaderAction(
        const DataSection& section,
        ParticleTintShaderAction& output)
    {
        if (!ReadActionCommon(
                section,
                output.common) ||
            !ReadOptionalBoolean(
                section,
                "repeat_",
                output.repeat) ||
            !ReadOptionalFloat(
                section,
                "period_",
                output.period) ||
            !ReadOptionalFloat(
                section,
                "fogAmount_",
                output.fogAmount))
        {
            return false;
        }

        const DataSection* tints =
            section.FindChild(
                "tints_");

        if (tints == nullptr)
        {
            return true;
        }

        for (const DataSection& tintSection :
             tints->children)
        {
            if (tintSection.name !=
                "Tint")
            {
                continue;
            }

            ParticleTintKey
                tint;

            if (!ReadOptionalFloat(
                    tintSection,
                    "time",
                    tint.time) ||
                !ReadOptionalArray(
                    tintSection,
                    "color",
                    tint.colour))
            {
                return false;
            }

            output.tints.push_back(
                tint);
        }

        return true;
    }

    bool ReadParticleAction(
        const DataSection& section,
        ParticleDefinition& definition,
        ParticleActionDefinition& output,
        std::string& error)
    {
        output = {};

        output.typeName =
            section.name;

        ++definition.statistics.actionCount;

        if (section.name ==
            "Source")
        {
            ParticleSourceAction
                action;

            if (!ReadSourceAction(
                    section,
                    definition,
                    action,
                    error))
            {
                return false;
            }

            output.type =
                ParticleActionType::Source;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Sink")
        {
            ParticleSinkAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalFloat(
                    section,
                    "maximumAge_",
                    action.maximumAge) ||
                !ReadOptionalFloat(
                    section,
                    "minimumSpeed_",
                    action.minimumSpeed) ||
                !ReadOptionalBoolean(
                    section,
                    "outsideOnly_",
                    action.outsideOnly))
            {
                error =
                    "Particle Sink action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Sink;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "TintShader")
        {
            ParticleTintShaderAction
                action;

            if (!ReadTintShaderAction(
                    section,
                    action))
            {
                error =
                    "Particle TintShader action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::TintShader;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Orbitor")
        {
            ParticleOrbitorAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalVector3(
                    section,
                    "point_",
                    action.point) ||
                !ReadOptionalFloat(
                    section,
                    "angularVelocity_",
                    action.angularVelocity) ||
                !ReadOptionalBoolean(
                    section,
                    "affectVelocity_",
                    action.affectVelocity))
            {
                error =
                    "Particle Orbitor action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Orbitor;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Jitter")
        {
            ParticleJitterAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalBoolean(
                    section,
                    "affectPosition_",
                    action.affectPosition) ||
                !ReadOptionalBoolean(
                    section,
                    "affectVelocity_",
                    action.affectVelocity))
            {
                error =
                    "Particle Jitter action contains invalid data.";

                return false;
            }

            if (!ReadVectorGenerator(
                    section.FindChild(
                        "pPositionSrc"),
                    action.positionSource,
                    definition.statistics,
                    error))
            {
                return false;
            }

            if (!ReadVectorGenerator(
                    section.FindChild(
                        "pVelocitySrc"),
                    action.velocitySource,
                    definition.statistics,
                    error))
            {
                return false;
            }

            output.type =
                ParticleActionType::Jitter;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Stream")
        {
            ParticleStreamAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalVector3(
                    section,
                    "vector_",
                    action.vector) ||
                !ReadOptionalFloat(
                    section,
                    "halfLife_",
                    action.halfLife))
            {
                error =
                    "Particle Stream action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Stream;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Force")
        {
            ParticleForceAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalVector3(
                    section,
                    "vector_",
                    action.vector))
            {
                error =
                    "Particle Force action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Force;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Magnet")
        {
            ParticleMagnetAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalFloat(
                    section,
                    "strength_",
                    action.strength) ||
                !ReadOptionalFloat(
                    section,
                    "minDist_",
                    action.minDistance))
            {
                error =
                    "Particle Magnet action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Magnet;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Barrier")
        {
            ParticleBarrierAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalInteger(
                    section,
                    "shape_",
                    action.shape) ||
                !ReadOptionalInteger(
                    section,
                    "reaction_",
                    action.reaction) ||
                !ReadOptionalVector3(
                    section,
                    "vecA_",
                    action.vectorA) ||
                !ReadOptionalVector3(
                    section,
                    "vecB_",
                    action.vectorB) ||
                !ReadOptionalFloat(
                    section,
                    "radius_",
                    action.radius))
            {
                error =
                    "Particle Barrier action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Barrier;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Scaler")
        {
            ParticleScalerAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalFloat(
                    section,
                    "size_",
                    action.size) ||
                !ReadOptionalFloat(
                    section,
                    "rate_",
                    action.rate))
            {
                error =
                    "Particle Scaler action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Scaler;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Flare")
        {
            ParticleFlareAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalString(
                    section,
                    "flareName_",
                    action.flareName) ||
                !ReadOptionalInteger(
                    section,
                    "flareStep_",
                    action.flareStep) ||
                !ReadOptionalBoolean(
                    section,
                    "colourize_",
                    action.colourize) ||
                !ReadOptionalBoolean(
                    section,
                    "useParticleSize_",
                    action.useParticleSize) ||
                !ReadOptionalBoolean(
                    section,
                    "bHasDirection_",
                    action.hasDirection) ||
                !ReadOptionalVector3(
                    section,
                    "direction_",
                    action.direction) ||
                !ReadOptionalFloat(
                    section,
                    "visibilityDotMin_",
                    action.visibilityDotMin) ||
                !ReadOptionalFloat(
                    section,
                    "visibilityDotMax_",
                    action.visibilityDotMax) ||
                !ReadOptionalFloat(
                    section,
                    "visibilityMinValue_",
                    action.visibilityMinValue) ||
                !ReadOptionalFloat(
                    section,
                    "visibilityMaxValue_",
                    action.visibilityMaxValue))
            {
                error =
                    "Particle Flare action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Flare;

            output.data =
                std::move(action);

            return true;
        }

        if (section.name ==
            "Collide")
        {
            ParticleCollideAction
                action;

            if (!ReadActionCommon(
                    section,
                    action.common) ||
                !ReadOptionalBoolean(
                    section,
                    "spriteBased_",
                    action.spriteBased) ||
                !ReadOptionalFloat(
                    section,
                    "elasticity_",
                    action.elasticity) ||
                !ReadOptionalFloat(
                    section,
                    "minAddedRotation_",
                    action.minAddedRotation) ||
                !ReadOptionalFloat(
                    section,
                    "maxAddedRotation_",
                    action.maxAddedRotation) ||
                !ReadOptionalInteger(
                    section,
                    "entityID_",
                    action.entityId) ||
                !ReadOptionalString(
                    section,
                    "soundTag_",
                    action.soundTag) ||
                !ReadOptionalBoolean(
                    section,
                    "soundEnabled_",
                    action.soundEnabled) ||
                !ReadOptionalInteger(
                    section,
                    "soundSrcIdx_",
                    action.soundSourceIndex) ||
                !ReadOptionalString(
                    section,
                    "soundProject_",
                    action.soundProject) ||
                !ReadOptionalString(
                    section,
                    "soundGroup_",
                    action.soundGroup) ||
                !ReadOptionalString(
                    section,
                    "soundName_",
                    action.soundName) ||
                !ReadOptionalBoolean(
                    section,
                    "cylinderCollide_",
                    action.cylinderCollide) ||
                !ReadOptionalFloat(
                    section,
                    "yOffset_",
                    action.yOffset) ||
                !ReadOptionalFloat(
                    section,
                    "frictionCoeff_",
                    action.frictionCoefficient))
            {
                error =
                    "Particle Collide action contains invalid data.";

                return false;
            }

            output.type =
                ParticleActionType::Collide;

            output.data =
                std::move(action);

            return true;
        }

        output.type =
            ParticleActionType::Unsupported;

        ++definition.statistics
            .unsupportedActionCount;

        return true;
    }

    bool ReadParticleRenderer(
        const core::resources::ResourceFileSystem& resources,
        const DataSection& rendererSection,
        ParticleDefinition& definition,
        ParticleRendererDefinition& output,
        std::string& error)
    {
        output = {};
        error.clear();

        if (rendererSection.children.empty())
        {
            return true;
        }

        const DataSection& section =
            rendererSection.children.front();

        ++definition.statistics.rendererCount;

        output.typeName =
            section.name;

        if (section.name ==
            "SpriteParticleRenderer")
        {
            output.type =
                ParticleRendererType::Sprite;
        }
        else if (section.name ==
                 "SpriteBlendRenderer")
        {
            output.type =
                ParticleRendererType::SpriteBlend;
        }
        else if (section.name ==
                 "TrailParticleRenderer")
        {
            output.type =
                ParticleRendererType::Trail;
        }
        else
        {
            output.type =
                ParticleRendererType::Unsupported;

            ++definition.statistics
                .unsupportedRendererCount;

            return true;
        }

        if (!ReadOptionalBoolean(
                section,
                "viewDependent_",
                output.viewDependent) ||
            !ReadOptionalBoolean(
                section,
                "local_",
                output.local))
        {
            error =
                "Particle renderer contains invalid common fields.";

            return false;
        }

        std::string textureReference;

        if (!ReadOptionalString(
                section,
                "textureName_",
                textureReference))
        {
            error =
                "Particle renderer contains invalid textureName_.";

            return false;
        }

        if (!textureReference.empty())
        {
            if (!ResolveTexture(
                    resources,
                    textureReference,
                    definition,
                    output.texture,
                    error))
            {
                return false;
            }
        }

        if (output.type ==
                ParticleRendererType::Sprite ||
            output.type ==
                ParticleRendererType::SpriteBlend)
        {
            if (!ReadOptionalInteger(
                    section,
                    "materialFX_",
                    output.materialFx) ||
                !ReadOptionalInteger(
                    section,
                    "frameCount_",
                    output.frameCount) ||
                !ReadOptionalFloat(
                    section,
                    "frameRate_",
                    output.frameRate) ||
                !ReadOptionalArray(
                    section,
                    "explicitOrientation_",
                    output.explicitOrientation) ||
                !ReadOptionalArray(
                    section,
                    "textureOffset_",
                    output.textureOffset) ||
                !ReadOptionalBoolean(
                    section,
                    "useFog_",
                    output.useFog))
            {
                error =
                    "Particle sprite renderer contains invalid data.";

                return false;
            }
        }

        if (output.type ==
            ParticleRendererType::SpriteBlend)
        {
            std::string normalMapReference;

            if (!ReadOptionalString(
                    section,
                    "normalMapName_",
                    normalMapReference))
            {
                error =
                    "Particle SpriteBlendRenderer contains invalid normal map.";

                return false;
            }

            if (!normalMapReference.empty())
            {
                if (!ResolveTexture(
                        resources,
                        normalMapReference,
                        definition,
                        output.normalMap,
                        error))
                {
                    return false;
                }
            }
        }

        if (output.type ==
            ParticleRendererType::Trail)
        {
            if (!ReadOptionalFloat(
                    section,
                    "width_",
                    output.width) ||
                !ReadOptionalInteger(
                    section,
                    "skip_",
                    output.skip) ||
                !ReadOptionalInteger(
                    section,
                    "steps",
                    output.steps))
            {
                error =
                    "Particle TrailParticleRenderer contains invalid data.";

                return false;
            }

            const DataSection* useFog =
                section.FindChild(
                    "useFog");

            if (useFog != nullptr)
            {
                if (!ReadBoolean(
                        *useFog,
                        output.useFog))
                {
                    error =
                        "Particle TrailParticleRenderer contains invalid useFog.";

                    return false;
                }
            }
            else if (!ReadOptionalBoolean(
                         section,
                         "useFog_",
                         output.useFog))
            {
                error =
                    "Particle TrailParticleRenderer contains invalid useFog_.";

                return false;
            }
        }

        return true;
    }

    bool ReadBoundingBox(
        const DataSection& section,
        core::math::BoundingBox& output)
    {
        const DataSection* minimum =
            section.FindChild(
                "min");

        const DataSection* maximum =
            section.FindChild(
                "max");

        if (minimum == nullptr ||
            maximum == nullptr)
        {
            return false;
        }

        return
            ReadVector3(
                *minimum,
                output.minimum) &&
            ReadVector3(
                *maximum,
                output.maximum);
    }

    bool ReadParticleSystem(
        const core::resources::ResourceFileSystem& resources,
        const DataSection& section,
        ParticleDefinition& definition,
        ParticleSystemDefinition& output,
        std::string& error)
    {
        output = {};

        output.name =
            section.name;

        if (!ReadOptionalInteger(
                section,
                "serialiseVersionData",
                output.serialiseVersion) ||
            !ReadOptionalFloat(
                section,
                "windFactor_",
                output.windFactor) ||
            !ReadOptionalBoolean(
                section,
                "bWindEnabled_",
                output.windEnabled) ||
            !ReadOptionalBoolean(
                section,
                "explicitTransform_",
                output.explicitTransform) ||
            !ReadOptionalVector3(
                section,
                "explicitPosition_",
                output.explicitPosition) ||
            !ReadOptionalVector3(
                section,
                "explicitDirection_",
                output.explicitDirection) ||
            !ReadOptionalVector3(
                section,
                "localOffset_",
                output.localOffset) ||
            !ReadOptionalFloat(
                section,
                "maxLod_",
                output.maxLod) ||
            !ReadOptionalFloat(
                section,
                "fixedFrameRate_",
                output.fixedFrameRate) ||
            !ReadOptionalInteger(
                section,
                "capacity",
                output.capacity))
        {
            error =
                "Particle system contains invalid root fields: " +
                section.name;

            return false;
        }

        const DataSection* boundingBox =
            section.FindChild(
                "boundingBox");

        if (boundingBox != nullptr)
        {
            if (!ReadBoundingBox(
                    *boundingBox,
                    output.boundingBox))
            {
                error =
                    "Particle system contains invalid boundingBox: " +
                    section.name;

                return false;
            }

            output.hasBoundingBox =
                true;
        }

        const DataSection* actions =
            section.FindChild(
                "Actions");

        if (actions != nullptr)
        {
            output.actions.reserve(
                actions->children.size());

            for (const DataSection& actionSection :
                 actions->children)
            {
                ParticleActionDefinition
                    action;

                if (!ReadParticleAction(
                        actionSection,
                        definition,
                        action,
                        error))
                {
                    error =
                        section.name +
                        ": " +
                        error;

                    return false;
                }

                output.actions.push_back(
                    std::move(action));
            }
        }

        const DataSection* renderer =
            section.FindChild(
                "Renderer");

        if (renderer != nullptr &&
            !renderer->children.empty())
        {
            if (!ReadParticleRenderer(
                    resources,
                    *renderer,
                    definition,
                    output.renderer,
                    error))
            {
                error =
                    section.name +
                    ": " +
                    error;

                return false;
            }

            output.hasRenderer =
                true;
        }

        return true;
    }
}

namespace core::world::particles
{
    bool ParticleLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view resourceReference,
        ParticleDefinition& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!resources.IsInitialized())
        {
            error =
                "Resource filesystem is not initialized.";

            return false;
        }

        const std::string logicalPath =
            BuildResourcePath(
                resources,
                resourceReference);

        if (logicalPath.empty())
        {
            error =
                "Particle resource reference is invalid: " +
                std::string(
                    resourceReference);

            return false;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "Particle resource not found: " +
                logicalPath;

            return false;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                logicalPath,
                encoded))
        {
            error =
                "Unable to read particle resource: " +
                logicalPath;

            return false;
        }

        resources::PackedSectionReader
            reader;

        resources::DataSection
            root;

        if (!reader.Read(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                root,
                error))
        {
            error =
                "Unable to parse particle resource " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        ParticleDefinition
            definition;

        definition.logicalPath =
            logicalPath;

        definition.systems.reserve(
            root.children.size());

        for (const resources::DataSection& section :
             root.children)
        {
            ParticleSystemDefinition
                system;

            if (!ReadParticleSystem(
                    resources,
                    section,
                    definition,
                    system,
                    error))
            {
                error =
                    logicalPath +
                    ": " +
                    error;

                return false;
            }

            definition.systems.push_back(
                std::move(system));
        }

        definition.statistics.systemCount =
            definition.systems.size();

        if (definition.systems.empty())
        {
            error =
                "Particle resource contains no particle systems: " +
                logicalPath;

            return false;
        }

        output =
            std::move(definition);

        return true;
    }
}