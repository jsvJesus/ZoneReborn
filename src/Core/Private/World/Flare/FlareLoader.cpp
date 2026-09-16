#include "Core/World/Flare/FlareLoader.h"

#include "Core/Assets/MaterialLoader.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace
{
    bool ReadScalar(
        const core::resources::DataSection& section,
        float& output)
    {
        if (section.TryGetFloat(
                output))
        {
            return true;
        }

        const std::string* text =
            section.AsString();

        if (text == nullptr ||
            text->empty())
        {
            return false;
        }

        errno =
            0;

        char* end =
            nullptr;

        const float value =
            std::strtof(
                text->c_str(),
                &end);

        if (errno != 0 ||
            end == text->c_str())
        {
            return false;
        }

        while (*end == ' ' ||
               *end == '\t' ||
               *end == '\r' ||
               *end == '\n')
        {
            ++end;
        }

        if (*end != '\0')
        {
            return false;
        }

        output =
            value;

        return true;
    }

    bool ReadRequiredScalar(
        const core::resources::DataSection& section,
        const std::string_view name,
        float& output)
    {
        const core::resources::DataSection* child =
            section.FindChild(
                name);

        if (child == nullptr)
        {
            return false;
        }

        return
            ReadScalar(
                *child,
                output);
    }

    bool ReadRgba(
        const core::resources::DataSection& section,
        std::array<float, 4>& output)
    {
        const core::resources::DataSection*
            child =
                section.FindChild(
                    "rgba");

        if (child == nullptr)
        {
            return true;
        }

        const core::resources::DataSection::FloatArray*
            values =
                child->AsFloats();

        if (values == nullptr ||
            values->size() != 4)
        {
            return false;
        }

        output =
        {
            (*values)[0],
            (*values)[1],
            (*values)[2],
            (*values)[3]
        };

        return true;
    }

    std::string BuildResourcePath(
        const std::string_view reference,
        const std::string_view defaultExtension)
    {
        std::string logicalPath =
            core::resources::ResourcePath::ToResPath(
                reference);

        if (logicalPath.empty())
        {
            return {};
        }

        std::filesystem::path
            path(
                logicalPath);

        if (!path.has_extension())
        {
            logicalPath +=
                defaultExtension;
        }

        return
            core::resources::ResourcePath::Normalize(
                logicalPath);
    }

    bool LoadMaterialTexture(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view materialReference,
        core::assets::TextureResource& output,
        std::string& error)
    {
        output = {};
        error.clear();

        const std::string logicalPath =
            BuildResourcePath(
                materialReference,
                ".mfm");

        if (logicalPath.empty())
        {
            error =
                "Flare material reference is invalid.";

            return false;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "Flare material not found: " +
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
                "Unable to read flare material: " +
                logicalPath;

            return false;
        }

        core::resources::PackedSectionReader
            reader;

        core::resources::DataSection
            root;

        if (!reader.Read(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                root,
                error))
        {
            error =
                "Unable to parse flare material " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        core::assets::MaterialLoader
            materialLoader;

        core::assets::VisualMaterial
            material;

        if (!materialLoader.Load(
                resources,
                root,
                material,
                error))
        {
            error =
                "Unable to load flare material " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        for (const core::assets::VisualMaterialProperty& property :
             material.properties)
        {
            if (!property.texture.has_value())
            {
                continue;
            }

            output =
                *property.texture;

            return true;
        }

        error =
            "Flare material contains no texture: " +
            logicalPath;

        return false;
    }

    bool ReadFlareElement(
        const core::resources::ResourceFileSystem& resources,
        const core::resources::DataSection& section,
        core::world::flare::FlareDefinition& definition,
        std::string& error)
    {
        const core::resources::DataSection*
            typeSection =
                section.FindChild(
                    "type");

        if (typeSection == nullptr)
        {
            error =
                "Flare element has no material type.";

            return false;
        }

        const std::string*
            type =
                typeSection->AsString();

        if (type == nullptr ||
            type->empty())
        {
            error =
                "Flare element material type is invalid.";

            return false;
        }

        core::world::flare::FlareElementDefinition
            element;

        element.materialReference =
            *type;

        if (!ReadRequiredScalar(
                section,
                "size",
                element.size))
        {
            error =
                "Flare element has invalid size.";

            return false;
        }

        if (!ReadRequiredScalar(
                section,
                "depth",
                element.depth))
        {
            error =
                "Flare element has invalid depth.";

            return false;
        }

        if (!ReadRgba(
                section,
                element.rgba))
        {
            error =
                "Flare element has invalid rgba.";

            return false;
        }

        if (!LoadMaterialTexture(
                resources,
                element.materialReference,
                element.texture,
                error))
        {
            return false;
        }

        definition.elements.push_back(
            std::move(
                element));

        const core::resources::DataSection*
            secondaries =
                section.FindChild(
                    "secondaries");

        if (secondaries == nullptr)
        {
            return true;
        }

        for (const core::resources::DataSection& secondary :
             secondaries->children)
        {
            if (secondary.name !=
                "Flare")
            {
                continue;
            }

            if (!ReadFlareElement(
                    resources,
                    secondary,
                    definition,
                    error))
            {
                return false;
            }
        }

        return true;
    }
}

namespace core::world::flare
{
    bool FlareLoader::Load(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view resourceReference,
        FlareDefinition& output,
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
                resourceReference,
                ".xml");

        if (logicalPath.empty())
        {
            error =
                "Flare resource reference is invalid.";

            return false;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "Flare resource not found: " +
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
                "Unable to read flare resource: " +
                logicalPath;

            return false;
        }

        core::resources::PackedSectionReader
            reader;

        core::resources::DataSection
            root;

        if (!reader.Read(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                root,
                error))
        {
            error =
                "Unable to parse flare resource " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        FlareDefinition
            definition;

        definition.logicalPath =
            logicalPath;

        for (const core::resources::DataSection& section :
             root.children)
        {
            if (section.name !=
                "Flare")
            {
                continue;
            }

            if (!ReadFlareElement(
                    resources,
                    section,
                    definition,
                    error))
            {
                error =
                    logicalPath +
                    ": " +
                    error;

                return false;
            }
        }

        if (definition.elements.empty())
        {
            error =
                "Flare resource contains no Flare elements: " +
                logicalPath;

            return false;
        }

        output =
            std::move(
                definition);

        return true;
    }
}