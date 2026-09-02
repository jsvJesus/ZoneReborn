#include "Core/World/SpaceLoader.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool ReadInteger(
        const core::resources::DataSection& parent,
        const std::string_view name,
        std::int32_t& output)
    {
        const core::resources::DataSection* section =
            parent.FindChild(name);

        if (section == nullptr)
        {
            return false;
        }

        const std::int64_t* value =
            section->AsInteger();

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

    bool ReadFloat(
        const core::resources::DataSection& parent,
        const std::string_view name,
        float& output)
    {
        const core::resources::DataSection* section =
            parent.FindChild(name);

        if (section == nullptr)
        {
            return false;
        }

        return section->TryGetFloat(output);
    }

    bool ReadString(
        const core::resources::DataSection& parent,
        const std::string_view name,
        std::string& output)
    {
        const core::resources::DataSection* section =
            parent.FindChild(name);

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

        output = *value;

        return true;
    }

    bool IsValidSpaceName(
        const std::string_view name)
    {
        if (name.empty())
        {
            return false;
        }

        if (name.find("..") !=
            std::string_view::npos)
        {
            return false;
        }

        if (name.find('/') !=
            std::string_view::npos)
        {
            return false;
        }

        if (name.find('\\') !=
            std::string_view::npos)
        {
            return false;
        }

        return true;
    }
}

namespace core::world
{
    bool SpaceLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view spaceName,
        SpaceSettings& output,
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

        if (!IsValidSpaceName(spaceName))
        {
            error =
                "Space name is invalid.";

            return false;
        }

        std::string resourcePath;

        resourcePath.reserve(
            28 +
            spaceName.size());

        resourcePath =
            "res/spaces/";

        resourcePath.append(
            spaceName);

        resourcePath +=
            "/space.settings";

        std::vector<std::byte> data;

        if (!resources.ReadBinary(
                resourcePath,
                data))
        {
            error =
                "Unable to read space.settings: " +
                resourcePath;

            return false;
        }

        resources::DataSection root;

        if (!reader_.Read(
                data,
                root,
                error))
        {
            error =
                "Unable to parse " +
                resourcePath +
                ": " +
                error;

            return false;
        }

        const resources::DataSection* bounds =
            root.FindChild("bounds");

        if (bounds == nullptr)
        {
            error =
                "space.settings does not contain bounds.";

            return false;
        }

        SpaceSettings settings;

        settings.name =
            std::string(spaceName);

        settings.resourcePath =
            resourcePath;

        if (!ReadInteger(
                *bounds,
                "minX",
                settings.bounds.minX) ||
            !ReadInteger(
                *bounds,
                "maxX",
                settings.bounds.maxX) ||
            !ReadInteger(
                *bounds,
                "minY",
                settings.bounds.minY) ||
            !ReadInteger(
                *bounds,
                "maxY",
                settings.bounds.maxY))
        {
            error =
                "space.settings contains invalid bounds.";

            return false;
        }

        if (settings.bounds.maxX <
                settings.bounds.minX ||
            settings.bounds.maxY <
                settings.bounds.minY)
        {
            error =
                "space.settings bounds are inverted.";

            return false;
        }

        const resources::DataSection* terrain =
            root.FindChild("terrain");

        if (terrain != nullptr)
        {
            ReadInteger(
                *terrain,
                "version",
                settings.terrain.version);

            ReadInteger(
                *terrain,
                "heightMapSize",
                settings.terrain.heightMapSize);

            ReadInteger(
                *terrain,
                "normalMapSize",
                settings.terrain.normalMapSize);

            ReadInteger(
                *terrain,
                "holeMapSize",
                settings.terrain.holeMapSize);

            ReadInteger(
                *terrain,
                "shadowMapSize",
                settings.terrain.shadowMapSize);

            ReadInteger(
                *terrain,
                "blendMapSize",
                settings.terrain.blendMapSize);
        }

        ReadFloat(
            root,
            "farPlane",
            settings.farPlane);

        ReadString(
            root,
            "timeOfDay",
            settings.timeOfDay);

        ReadString(
            root,
            "skyGradientDome",
            settings.skyGradientDome);

        output =
            std::move(settings);

        return true;
    }
}