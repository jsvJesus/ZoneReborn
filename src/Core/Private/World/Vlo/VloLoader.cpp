#include "Core/World/Vlo/VloLoader.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/ResourcePath.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using DataSection =
        core::resources::DataSection;

    bool SetInvalidFieldError(
        const std::string_view field,
        std::string& error)
    {
        error =
            "Invalid or missing VLO field: ";

        error.append(
            field);

        return false;
    }

    bool ReadFloat(
        const DataSection& parent,
        const std::string_view name,
        float& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        if (!section->TryGetFloat(
                output))
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        return true;
    }

    template<std::size_t Count>
    bool ReadFloatArray(
        const DataSection& parent,
        const std::string_view name,
        std::array<float, Count>& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        const DataSection::FloatArray* values =
            section->AsFloats();

        if (values == nullptr ||
            values->size() != Count)
        {
            return SetInvalidFieldError(
                name,
                error);
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
        const DataSection& parent,
        const std::string_view name,
        core::math::Vector3& output,
        std::string& error)
    {
        std::array<float, 3>
            values{};

        if (!ReadFloatArray(
                parent,
                name,
                values,
                error))
        {
            return false;
        }

        output.x =
            values[0];

        output.y =
            values[1];

        output.z =
            values[2];

        return true;
    }

    bool ReadString(
        const DataSection& parent,
        const std::string_view name,
        std::string& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        const std::string* value =
            section->AsString();

        if (value == nullptr ||
            value->empty())
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        output =
            *value;

        return true;
    }

    bool ReadBoolean(
        const DataSection& parent,
        const std::string_view name,
        bool& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        const bool* value =
            section->AsBoolean();

        if (value == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        output =
            *value;

        return true;
    }

    bool ReadOptionalBoolean(
        const DataSection& parent,
        const std::string_view name,
        bool& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return true;
        }

        const bool* value =
            section->AsBoolean();

        if (value == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        output =
            *value;

        return true;
    }

    bool ReadInteger(
        const DataSection& parent,
        const std::string_view name,
        std::int32_t& output,
        std::string& error)
    {
        const DataSection* section =
            parent.FindChild(
                name);

        if (section == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        const std::int64_t* value =
            section->AsInteger();

        if (value == nullptr)
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        if (*value <
                std::numeric_limits<
                    std::int32_t>::min() ||
            *value >
                std::numeric_limits<
                    std::int32_t>::max())
        {
            return SetInvalidFieldError(
                name,
                error);
        }

        output =
            static_cast<std::int32_t>(
                *value);

        return true;
    }

    bool ReadTexture(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::TextureResolver& resolver,
        const DataSection& parent,
        const std::string_view name,
        core::assets::TextureResource& output,
        std::string& error)
    {
        std::string reference;

        if (!ReadString(
                parent,
                name,
                reference,
                error))
        {
            return false;
        }

        if (!resolver.Resolve(
                resources,
                reference,
                output))
        {
            error =
                "Unable to resolve VLO texture ";

            error.append(
                name);

            error +=
                ": ";

            error +=
                reference;

            return false;
        }

        return true;
    }

    bool ReadWater(
        const core::resources::ResourceFileSystem& resources,
        const DataSection& section,
        core::world::water::WaterDefinition& output,
        std::string& error)
    {
        core::assets::TextureResolver
            textureResolver;

        if (!ReadVector3(
                section,
                "position",
                output.position,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "orientation",
                output.orientation,
                error))
        {
            return false;
        }

        if (!ReadVector3(
                section,
                "size",
                output.size,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "fresnelConstant",
                output.fresnelConstant,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "fresnelExponent",
                output.fresnelExponent,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "reflectionTint",
                output.reflectionTint,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "reflectionStrength",
                output.reflectionStrength,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "refractionTint",
                output.refractionTint,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "refractionStrength",
                output.refractionStrength,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "tessellation",
                output.tessellation,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "consistency",
                output.consistency,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "textureTessellation",
                output.textureTessellation,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "scrollSpeed1",
                output.scrollSpeed1,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "scrollSpeed2",
                output.scrollSpeed2,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "waveScale",
                output.waveScale,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "windVelocity",
                output.windVelocity,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "sunPower",
                output.sunPower,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "sunScale",
                output.sunScale,
                error))
        {
            return false;
        }

        if (!ReadTexture(
                resources,
                textureResolver,
                section,
                "waveTexture",
                output.waveTexture,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "cellsize",
                output.cellSize,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "smoothness",
                output.smoothness,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "depth",
                output.depth,
                error))
        {
            return false;
        }

        if (!ReadTexture(
                resources,
                textureResolver,
                section,
                "foamTexture",
                output.foamTexture,
                error))
        {
            return false;
        }

        if (!ReadTexture(
                resources,
                textureResolver,
                section,
                "reflectionTexture",
                output.reflectionTexture,
                error))
        {
            return false;
        }

        if (!ReadFloatArray(
                section,
                "deepColour",
                output.deepColour,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "fadeDepth",
                output.fadeDepth,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "foamIntersection",
                output.foamIntersection,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "foamMultiplier",
                output.foamMultiplier,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "foamTiling",
                output.foamTiling,
                error))
        {
            return false;
        }

        if (!ReadBoolean(
                section,
                "bypassDepth",
                output.bypassDepth,
                error))
        {
            return false;
        }

        if (!ReadBoolean(
                section,
                "useCubeMap",
                output.useCubeMap,
                error))
        {
            return false;
        }

        if (!ReadBoolean(
                section,
                "useEdgeAlpha",
                output.useEdgeAlpha,
                error))
        {
            return false;
        }

        if (!ReadBoolean(
                section,
                "useSimulation",
                output.useSimulation,
                error))
        {
            return false;
        }

        if (!ReadInteger(
                section,
                "visibility",
                output.visibility,
                error))
        {
            return false;
        }

        if (!ReadInteger(
                section,
                "depthStr",
                output.depthStr,
                error))
        {
            return false;
        }

        if (!ReadFloat(
                section,
                "physicDepth",
                output.physicDepth,
                error))
        {
            return false;
        }

        return true;
    }

    bool ReadEditorOnly(
        const DataSection& root,
        core::world::vlo::VloResource& output,
        std::string& error)
    {
        const DataSection* section =
            root.FindChild(
                "editorOnly");

        if (section == nullptr)
        {
            return true;
        }

        if (!ReadOptionalBoolean(
                *section,
                "hidden",
                output.hidden,
                error))
        {
            return false;
        }

        if (!ReadOptionalBoolean(
                *section,
                "frozen",
                output.frozen,
                error))
        {
            return false;
        }

        return true;
    }
}

namespace core::world::vlo
{
    bool VloLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view logicalPath,
        const std::string_view expectedType,
        VloResource& output,
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

        const std::string normalizedPath =
            resources::ResourcePath::ToResPath(
                logicalPath);

        if (normalizedPath.empty())
        {
            error =
                "VLO path is invalid.";

            return false;
        }

        if (!normalizedPath.ends_with(
                ".vlo"))
        {
            error =
                "VLO resource does not have .vlo extension: " +
                normalizedPath;

            return false;
        }

        std::vector<std::byte>
            binary;

        if (!resources.ReadBinary(
                normalizedPath,
                binary))
        {
            error =
                "Unable to read VLO resource: " +
                normalizedPath;

            return false;
        }

        resources::DataSection
            root;

        if (!reader_.Read(
                binary,
                root,
                error))
        {
            error =
                "Unable to parse VLO resource " +
                normalizedPath +
                ": " +
                error;

            return false;
        }

        const std::string normalizedType =
            resources::ResourcePath::Normalize(
                expectedType);

        if (normalizedType.empty())
        {
            error =
                "VLO type is invalid.";

            return false;
        }

        VloResource resource;

        resource.logicalPath =
            normalizedPath;

        resource.typeName =
            normalizedType;

        if (normalizedType ==
            "water")
        {
            const resources::DataSection*
                waterSection =
                    root.FindChild(
                        "water");

            if (waterSection == nullptr)
            {
                error =
                    "VLO resource is referenced as water but does not contain water section: " +
                    normalizedPath;

                return false;
            }

            water::WaterDefinition
                definition;

            if (!ReadWater(
                    resources,
                    *waterSection,
                    definition,
                    error))
            {
                error =
                    "Unable to read water VLO " +
                    normalizedPath +
                    ": " +
                    error;

                return false;
            }

            resource.type =
                VloType::Water;

            resource.water =
                std::move(
                    definition);
        }
        else
        {
            error =
                "Unsupported VLO type: " +
                normalizedType;

            return false;
        }

        if (!ReadEditorOnly(
                root,
                resource,
                error))
        {
            error =
                "Unable to read VLO editor flags " +
                normalizedPath +
                ": " +
                error;

            return false;
        }

        output =
            std::move(
                resource);

        return true;
    }
}