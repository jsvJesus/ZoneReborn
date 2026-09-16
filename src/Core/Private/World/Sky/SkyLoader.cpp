#include "Core/World/Sky/SkyLoader.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Resources/ResourcePath.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool ReadFloat(
        const core::resources::DataSection& parent,
        const std::string_view name,
        float& output)
    {
        const core::resources::DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return false;
        }

        return
            section->TryGetFloat(
                output);
    }

    bool ReadString(
        const core::resources::DataSection& parent,
        const std::string_view name,
        std::string& output)
    {
        const core::resources::DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return false;
        }

        const std::string* value =
            section->AsString();

        if (value == nullptr)
        {
            return false;
        }

        output =
            *value;

        return true;
    }

    bool ReadColourKey(
        const core::resources::DataSection& section,
        core::world::sky::SkyColourKey& output)
    {
        if (!ReadFloat(
                section,
                "time",
                output.time))
        {
            return false;
        }

        const core::resources::DataSection* colour =
            section.FindChild(
                "colour");

        if (colour == nullptr)
        {
            return false;
        }

        const core::resources::DataSection::FloatArray* values =
            colour->AsFloats();

        if (values == nullptr ||
            values->size() != 3)
        {
            return false;
        }

        output.colour.x =
            (*values)[0];

        output.colour.y =
            (*values)[1];

        output.colour.z =
            (*values)[2];

        if (!std::isfinite(
                output.time) ||
            !std::isfinite(
                output.colour.x) ||
            !std::isfinite(
                output.colour.y) ||
            !std::isfinite(
                output.colour.z))
        {
            return false;
        }

        return true;
    }

    bool ReadKeys(
        const core::resources::DataSection& cycle,
        const std::string_view name,
        std::vector<
            core::world::sky::SkyColourKey>& output)
    {
        output.clear();

        const std::vector<
            const core::resources::DataSection*>
            sections =
                cycle.FindChildren(
                    name);

        output.reserve(
            sections.size());

        for (const core::resources::DataSection* section :
             sections)
        {
            if (section == nullptr)
            {
                return false;
            }

            core::world::sky::SkyColourKey
                key;

            if (!ReadColourKey(
                    *section,
                    key))
            {
                return false;
            }

            output.push_back(
                key);
        }

        std::stable_sort(
            output.begin(),
            output.end(),
            [](
                const core::world::sky::SkyColourKey& left,
                const core::world::sky::SkyColourKey& right)
            {
                return
                    left.time <
                    right.time;
            });

        return
            !output.empty();
    }
}

namespace core::world::sky
{
    bool SkyLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view resourceReference,
        SkyDefinition& output,
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
            resources::ResourcePath::ToResPath(
                resourceReference);

        if (logicalPath.empty())
        {
            error =
                "Sky resource path is invalid.";

            return false;
        }

        std::vector<std::byte>
            data;

        if (!resources.ReadBinary(
                logicalPath,
                data))
        {
            error =
                "Unable to read sky resource: " +
                logicalPath;

            return false;
        }

        resources::DataSection
            root;

        if (!reader_.Read(
                std::span<const std::byte>(
                    data.data(),
                    data.size()),
                root,
                error))
        {
            error =
                "Unable to parse sky resource " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        SkyDefinition
            definition;

        definition.resourcePath =
            logicalPath;

        std::string
            textureReference;

        if (!ReadString(
                root,
                "texture",
                textureReference))
        {
            error =
                "Sky resource does not contain texture.";

            return false;
        }

        assets::TextureResolver
            textureResolver;

        if (!textureResolver.Resolve(
                resources,
                textureReference,
                definition.gradientTexture))
        {
            error =
                "Unable to resolve sky gradient texture: " +
                textureReference;

            return false;
        }

        ReadFloat(
            root,
            "mieAmount",
            definition.mieAmount);

        ReadFloat(
            root,
            "turbidityOffset",
            definition.turbidityOffset);

        ReadFloat(
            root,
            "turbidityFactor",
            definition.turbidityFactor);

        ReadFloat(
            root,
            "vertexHeightEffect",
            definition.vertexHeightEffect);

        ReadFloat(
            root,
            "sunHeightEffect",
            definition.sunHeightEffect);

        ReadFloat(
            root,
            "power",
            definition.power);

        ReadFloat(
            root,
            "farPlane",
            definition.farPlane);

        const resources::DataSection* cycle =
            root.FindChild(
                "day_night_cycle");

        if (cycle == nullptr)
        {
            error =
                "Sky resource does not contain day_night_cycle.";

            return false;
        }

        ReadFloat(
            *cycle,
            "angle",
            definition.sunAngleDegrees);

        ReadFloat(
            *cycle,
            "moonAngle",
            definition.moonAngleDegrees);

        ReadFloat(
            *cycle,
            "hourlength",
            definition.hourLengthSeconds);

        ReadFloat(
            *cycle,
            "starttime",
            definition.startTimeHours);

        if (!std::isfinite(
                definition.hourLengthSeconds) ||
            definition.hourLengthSeconds <=
                0.0001f)
        {
            error =
                "Sky hourlength is invalid.";

            return false;
        }

        if (!std::isfinite(
                definition.startTimeHours))
        {
            error =
                "Sky starttime is invalid.";

            return false;
        }

        if (!ReadKeys(
                *cycle,
                "lightkey",
                definition.lightKeys))
        {
            error =
                "Sky lightkey list is invalid.";

            return false;
        }

        if (!ReadKeys(
                *cycle,
                "ambientkey",
                definition.ambientKeys))
        {
            error =
                "Sky ambientkey list is invalid.";

            return false;
        }

        output =
            std::move(
                definition);

        return true;
    }
}