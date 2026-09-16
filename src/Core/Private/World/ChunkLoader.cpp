#include "Core/World/ChunkLoader.h"

#include "Core/Resources/DataSection.h"

#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool IsValidPathComponent(
        const std::string_view value)
    {
        if (value.empty())
        {
            return false;
        }

        if (value.find("..") !=
            std::string_view::npos)
        {
            return false;
        }

        if (value.find('/') !=
            std::string_view::npos)
        {
            return false;
        }

        if (value.find('\\') !=
            std::string_view::npos)
        {
            return false;
        }

        return true;
    }

    bool ReadRequiredString(
        const core::resources::DataSection& section,
        const std::string_view childName,
        std::string& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return false;
        }

        const std::string* value =
            child->AsString();

        if (value == nullptr)
        {
            return false;
        }

        output = *value;

        return true;
    }

    bool ReadOptionalBoolean(
        const core::resources::DataSection& section,
        const std::string_view childName,
        bool& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return true;
        }

        const bool* value =
            child->AsBoolean();

        if (value == nullptr)
        {
            return false;
        }

        output = *value;

        return true;
    }

    bool ReadOptionalString(
        const core::resources::DataSection& section,
        const std::string_view childName,
        std::string& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return true;
        }

        const std::string* value =
            child->AsString();

        if (value == nullptr)
        {
            return false;
        }

        output = *value;

        return true;
    }

    bool ReadOptionalInteger(
        const core::resources::DataSection& section,
        const std::string_view childName,
        std::int32_t& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return true;
        }

        const std::int64_t* value =
            child->AsInteger();

        if (value == nullptr)
        {
            return false;
        }

        if (*value <
                std::numeric_limits<std::int32_t>::min() ||
            *value >
                std::numeric_limits<std::int32_t>::max())
        {
            return false;
        }

        output =
            static_cast<std::int32_t>(
                *value);

        return true;
    }

    bool ReadRequiredFloat(
        const core::resources::DataSection& section,
        const std::string_view childName,
        float& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return false;
        }

        return child->TryGetFloat(
            output);
    }

    bool ReadOptionalFloat(
        const core::resources::DataSection& section,
        const std::string_view childName,
        float& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return true;
        }

        return child->TryGetFloat(
            output);
    }

    bool ReadRequiredInteger(
        const core::resources::DataSection& section,
        const std::string_view childName,
        std::int32_t& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return false;
        }

        const std::int64_t* value =
            child->AsInteger();

        if (value == nullptr)
        {
            return false;
        }

        if (*value <
                std::numeric_limits<std::int32_t>::min() ||
            *value >
                std::numeric_limits<std::int32_t>::max())
        {
            return false;
        }

        output =
            static_cast<std::int32_t>(
                *value);

        return true;
    }

    bool ReadTransform(
        const core::resources::DataSection& section,
        core::math::Transform3x4& output)
    {
        const core::resources::DataSection::FloatArray*
            values =
                section.AsFloats();

        if (values == nullptr)
        {
            return false;
        }

        if (values->size() != 12)
        {
            return false;
        }

        for (std::size_t index = 0;
             index < output.values.size();
             ++index)
        {
            output.values[index] =
                (*values)[index];
        }

        return true;
    }

    bool ReadRequiredTransform(
        const core::resources::DataSection& section,
        const std::string_view childName,
        core::math::Transform3x4& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return false;
        }

        return ReadTransform(
            *child,
            output);
    }

    bool ReadVector3(
        const core::resources::DataSection& section,
        core::math::Vector3& output)
    {
        const core::resources::DataSection::FloatArray*
            values =
                section.AsFloats();

        if (values == nullptr)
        {
            return false;
        }

        if (values->size() != 3)
        {
            return false;
        }

        output.x = (*values)[0];
        output.y = (*values)[1];
        output.z = (*values)[2];

        return true;
    }

    bool ReadRequiredVector3(
        const core::resources::DataSection& section,
        const std::string_view childName,
        core::math::Vector3& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(childName);

        if (child == nullptr)
        {
            return false;
        }

        return ReadVector3(
            *child,
            output);
    }

    bool ReadBoundingBox(
        const core::resources::DataSection& section,
        core::math::BoundingBox& output)
    {
        const core::resources::DataSection* minimum =
            section.FindChild("min");

        const core::resources::DataSection* maximum =
            section.FindChild("max");

        if (minimum == nullptr ||
            maximum == nullptr)
        {
            return false;
        }

        if (!ReadVector3(
                *minimum,
                output.minimum))
        {
            return false;
        }

        if (!ReadVector3(
                *maximum,
                output.maximum))
        {
            return false;
        }

        return true;
    }

    bool ReadModel(
        const core::resources::DataSection& section,
        core::world::ChunkModelInstance& output)
    {
        if (!ReadRequiredString(
                section,
                "resource",
                output.resource))
        {
            return false;
        }

        if (!ReadRequiredTransform(
                section,
                "transform",
                output.transform))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "reflectionVisible",
                output.reflectionVisible))
        {
            return false;
        }

        return true;
    }

    bool ReadSpeedTree(
        const core::resources::DataSection& section,
        core::world::ChunkSpeedTreeInstance& output)
    {
        if (!ReadRequiredString(
                section,
                "spt",
                output.resource))
        {
            return false;
        }

        if (!ReadRequiredInteger(
                section,
                "seed",
                output.seed))
        {
            return false;
        }

        if (!ReadRequiredTransform(
                section,
                "transform",
                output.transform))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "reflectionVisible",
                output.reflectionVisible))
        {
            return false;
        }

        return true;
    }

    bool ReadTerrain(
        const core::resources::DataSection& section,
        core::world::ChunkTerrainReference& output)
    {
        return ReadRequiredString(
            section,
            "resource",
            output.resource);
    }

    bool ReadLargeObject(
        const core::resources::DataSection& section,
        core::world::ChunkLargeObjectReference& output)
    {
        if (!ReadRequiredString(
                section,
                "uid",
                output.uid))
        {
            return false;
        }

        if (!ReadRequiredString(
                section,
                "type",
                output.type))
        {
            return false;
        }

        return true;
    }

    bool ReadOmniLight(
        const core::resources::DataSection& section,
        core::world::ChunkOmniLight& output)
    {
        output = {};

        output.multiplier =
            1.0f;

        if (!ReadRequiredVector3(
                section,
                "colour",
                output.colour))
        {
            return false;
        }

        if (!ReadRequiredVector3(
                section,
                "position",
                output.position))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "innerRadius",
                output.innerRadius))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "outerRadius",
                output.outerRadius))
        {
            return false;
        }

        if (!ReadOptionalFloat(
                section,
                "multiplier",
                output.multiplier))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "dynamic",
                output.isDynamic))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "static",
                output.isStatic))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "specular",
                output.specular))
        {
            return false;
        }

        if (!ReadOptionalInteger(
                section,
                "priority",
                output.priority))
        {
            return false;
        }

        if (!ReadOptionalInteger(
                section,
                "lightType",
                output.lightType))
        {
            return false;
        }

        if (!ReadOptionalString(
                section,
                "guid",
                output.guid))
        {
            return false;
        }

        return true;
    }

    bool ReadSpotLight(
        const core::resources::DataSection& section,
        core::world::ChunkSpotLight& output)
    {
        output = {};

        output.multiplier =
            1.0f;

        output.cosConeAngle =
            1.0f;

        if (!ReadRequiredVector3(
                section,
                "colour",
                output.colour))
        {
            return false;
        }

        if (!ReadRequiredVector3(
                section,
                "position",
                output.position))
        {
            return false;
        }

        if (!ReadRequiredVector3(
                section,
                "direction",
                output.direction))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "innerRadius",
                output.innerRadius))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "outerRadius",
                output.outerRadius))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "cosConeAngle",
                output.cosConeAngle))
        {
            return false;
        }

        if (!ReadOptionalFloat(
                section,
                "multiplier",
                output.multiplier))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "dynamic",
                output.isDynamic))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "static",
                output.isStatic))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "specular",
                output.specular))
        {
            return false;
        }

        if (!ReadOptionalInteger(
                section,
                "priority",
                output.priority))
        {
            return false;
        }

        if (!ReadOptionalInteger(
                section,
                "lightType",
                output.lightType))
        {
            return false;
        }

        if (!ReadOptionalString(
                section,
                "guid",
                output.guid))
        {
            return false;
        }

        return true;
    }

    bool ReadPulseLightFrame(
        const core::resources::DataSection& section,
    core::animation::ScalarAnimationFrame& output)
    {
        const core::resources::DataSection::FloatArray*
            values =
                section.AsFloats();

        if (values == nullptr ||
            values->size() != 2)
        {
            return false;
        }

        output.time =
            (*values)[0];

        output.value =
            (*values)[1];

        return true;
    }

    bool ReadPulseLight(
        const core::resources::DataSection& section,
        core::world::ChunkPulseLight& output)
    {
        output = {};

        output.multiplier =
            1.0f;

        output.animation.timeScale =
            1.0f;

        if (!ReadRequiredVector3(
                section,
                "colour",
                output.colour))
        {
            return false;
        }

        if (!ReadRequiredVector3(
                section,
                "position",
                output.position))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "innerRadius",
                output.innerRadius))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "outerRadius",
                output.outerRadius))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "multiplier",
                output.multiplier))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "timeScale",
                output.animation.timeScale))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "duration",
                output.animation.duration))
        {
            return false;
        }

        if (!ReadRequiredString(
                section,
                "animation",
                output.animation.resource))
        {
            return false;
        }

        if (!ReadRequiredInteger(
                section,
                "priority",
                output.priority))
        {
            return false;
        }

        if (!ReadOptionalString(
                section,
                "guid",
                output.guid))
        {
            return false;
        }

        if (!std::isfinite(
                output.animation.timeScale) ||
            !std::isfinite(
                output.animation.duration))
        {
            return false;
        }

        const std::vector<
            const core::resources::DataSection*>
            frameSections =
                section.FindChildren(
                    "frame");

        if (frameSections.empty())
        {
            return false;
        }

        output.animation.frames.reserve(
            frameSections.size());

        float previousTime =
            -std::numeric_limits<float>::infinity();

        for (const core::resources::DataSection* frameSection :
             frameSections)
        {
            if (frameSection == nullptr)
            {
                return false;
            }

            core::animation::ScalarAnimationFrame
                frame;

            if (!ReadPulseLightFrame(
                    *frameSection,
                    frame))
            {
                return false;
            }

            if (!std::isfinite(
                    frame.time) ||
                !std::isfinite(
                    frame.value))
            {
                return false;
            }

            if (frame.time <
                previousTime)
            {
                return false;
            }

            previousTime =
                frame.time;

            output.animation.frames.push_back(
                frame);
        }

        return true;
    }

    bool ReadParticle(
        const core::resources::DataSection& section,
        core::world::ChunkParticleInstance& output)
    {
        output = {};

        if (!ReadRequiredString(
                section,
                "resource",
                output.resource))
        {
            return false;
        }

        if (!ReadRequiredTransform(
                section,
                "transform",
                output.transform))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                section,
                "reflectionVisible",
                output.reflectionVisible))
        {
            return false;
        }

        return true;
    }

    bool ReadFlare(
        const core::resources::DataSection& section,
        core::world::ChunkFlare& output)
    {
        output = {};

        output.colour =
        {
            255.0f,
            255.0f,
            255.0f
        };

        output.area =
            1.0f;

        output.fadeSpeed =
            1.0f;

        if (!ReadRequiredString(
                section,
                "resource",
                output.resource))
        {
            return false;
        }

        if (!ReadRequiredVector3(
                section,
                "position",
                output.position))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "maxDistance",
                output.maxDistance))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "area",
                output.area))
        {
            return false;
        }

        if (!ReadRequiredFloat(
                section,
                "fadeSpeed",
                output.fadeSpeed))
        {
            return false;
        }

        if (const core::resources::DataSection* colour =
                section.FindChild(
                    "colour"))
        {
            if (!ReadVector3(
                    *colour,
                    output.colour))
            {
                return false;
            }
        }

        if (!ReadOptionalString(
                section,
                "guid",
                output.guid))
        {
            return false;
        }

        return true;
    }
}

namespace core::world
{
    bool ChunkLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view spaceName,
        const std::string_view chunkId,
        Chunk& output,
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

        if (!IsValidPathComponent(spaceName))
        {
            error =
                "Space name is invalid.";

            return false;
        }

        if (!IsValidPathComponent(chunkId))
        {
            error =
                "Chunk id is invalid.";

            return false;
        }

        std::string resourcePath =
            "res/spaces/";

        resourcePath.append(
            spaceName);

        resourcePath.push_back('/');

        resourcePath.append(
            chunkId);

        resourcePath +=
            ".chunk";

        std::vector<std::byte> binary;

        if (!resources.ReadBinary(
                resourcePath,
                binary))
        {
            error =
                "Unable to read chunk: " +
                resourcePath;

            return false;
        }

        resources::DataSection root;

        if (!reader_.Read(
                binary,
                root,
                error))
        {
            error =
                "Unable to parse chunk " +
                resourcePath +
                ": " +
                error;

            return false;
        }

        Chunk chunk;

        chunk.spaceName =
            std::string(spaceName);

        chunk.chunkId =
            std::string(chunkId);

        chunk.resourcePath =
            resourcePath;

        if (const resources::DataSection* transform =
                root.FindChild("transform"))
        {
            math::Transform3x4 value;

            if (!ReadTransform(
                    *transform,
                    value))
            {
                error =
                    "Chunk contains invalid root transform: " +
                    resourcePath;

                return false;
            }

            chunk.transform =
                value;
        }

        if (const resources::DataSection* boundingBox =
                root.FindChild("boundingBox"))
        {
            math::BoundingBox value;

            if (!ReadBoundingBox(
                    *boundingBox,
                    value))
            {
                error =
                    "Chunk contains invalid boundingBox: " +
                    resourcePath;

                return false;
            }

            chunk.boundingBox =
                value;
        }

        for (const resources::DataSection& section :
             root.children)
        {
            if (section.name == "model")
            {
                ChunkModelInstance model;

                if (!ReadModel(
                        section,
                        model))
                {
                    error =
                        "Chunk contains invalid model section: " +
                        resourcePath;

                    return false;
                }

                chunk.models.push_back(
                    std::move(model));

                continue;
            }

            if (section.name == "shell")
            {
                ChunkModelInstance shell;

                if (!ReadModel(
                        section,
                        shell))
                {
                    error =
                        "Chunk contains invalid shell section: " +
                        resourcePath;

                    return false;
                }

                chunk.shells.push_back(
                    std::move(shell));

                continue;
            }

            if (section.name == "speedtree")
            {
                ChunkSpeedTreeInstance tree;

                if (!ReadSpeedTree(
                        section,
                        tree))
                {
                    error =
                        "Chunk contains invalid speedtree section: " +
                        resourcePath;

                    return false;
                }

                chunk.speedTrees.push_back(
                    std::move(tree));

                continue;
            }

            if (section.name == "omniLight")
            {
                ChunkOmniLight light;

                if (!ReadOmniLight(
                        section,
                        light))
                {
                    error =
                        "Chunk contains invalid omniLight section: " +
                        resourcePath;

                    return false;
                }

                chunk.omniLights.push_back(
                    std::move(light));

                continue;
            }

            if (section.name == "spotLight")
            {
                ChunkSpotLight light;

                if (!ReadSpotLight(
                        section,
                        light))
                {
                    error =
                        "Chunk contains invalid spotLight section: " +
                        resourcePath;

                    return false;
                }

                chunk.spotLights.push_back(
                    std::move(light));

                continue;
            }

            if (section.name == "pulseLight")
            {
                ChunkPulseLight light;

                if (!ReadPulseLight(
                        section,
                        light))
                {
                    error =
                        "Chunk contains invalid pulseLight section: " +
                        resourcePath;

                    return false;
                }

                chunk.pulseLights.push_back(
                    std::move(light));

                continue;
            }

            if (section.name == "flare")
            {
                ChunkFlare flare;

                if (!ReadFlare(
                        section,
                        flare))
                {
                    error =
                        "Chunk contains invalid flare section: " +
                        resourcePath;

                    return false;
                }

                chunk.flares.push_back(
                    std::move(
                        flare));

                continue;
            }

            if (section.name == "particles")
            {
                ChunkParticleInstance particle;

                if (!ReadParticle(
                        section,
                        particle))
                {
                    error =
                        "Chunk contains invalid particles section: " +
                        resourcePath;

                    return false;
                }

                chunk.particles.push_back(
                    std::move(
                        particle));

                continue;
            }

            if (section.name == "terrain")
            {
                ChunkTerrainReference terrain;

                if (!ReadTerrain(
                        section,
                        terrain))
                {
                    error =
                        "Chunk contains invalid terrain section: " +
                        resourcePath;

                    return false;
                }

                chunk.terrains.push_back(
                    std::move(terrain));

                continue;
            }

            if (section.name == "vlo")
            {
                ChunkLargeObjectReference object;

                if (!ReadLargeObject(
                        section,
                        object))
                {
                    error =
                        "Chunk contains invalid vlo section: " +
                        resourcePath;

                    return false;
                }

                chunk.largeObjects.push_back(
                    std::move(object));

                continue;
            }

            if (section.name == "overlapper")
            {
                const std::string* value =
                    section.AsString();

                if (value == nullptr)
                {
                    error =
                        "Chunk contains invalid overlapper section: " +
                        resourcePath;

                    return false;
                }

                chunk.overlappers.push_back(
                    *value);

                continue;
            }
        }

        output =
            std::move(chunk);

        return true;
    }
}