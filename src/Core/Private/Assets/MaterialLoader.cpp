#include "Core/Assets/MaterialLoader.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Log.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::size_t MaximumInheritanceDepth =
        8;

    std::string ToLower(
        std::string value)
    {
        for (char& character : value)
        {
            if (character >= 'A' &&
                character <= 'Z')
            {
                character =
                    static_cast<char>(
                        character -
                        'A' +
                        'a');
            }
        }

        return value;
    }

    bool ReadInt32(
        const core::resources::DataSection& section,
        std::int32_t& output)
    {
        const std::int64_t* value =
            section.AsInteger();

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

    std::string EncodeBase64(
        const std::vector<std::byte>& data)
    {
        static constexpr char Alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string output;

        output.reserve(
            ((data.size() + 2) / 3) * 4);

        for (std::size_t offset = 0;
             offset < data.size();
             offset += 3)
        {
            const std::uint32_t first =
                std::to_integer<unsigned char>(
                    data[offset]);

            const bool hasSecond =
                offset + 1 < data.size();

            const bool hasThird =
                offset + 2 < data.size();

            const std::uint32_t second =
                hasSecond
                    ? std::to_integer<unsigned char>(
                          data[offset + 1])
                    : 0;

            const std::uint32_t third =
                hasThird
                    ? std::to_integer<unsigned char>(
                          data[offset + 2])
                    : 0;

            const std::uint32_t value =
                (first << 16) |
                (second << 8) |
                third;

            output.push_back(
                Alphabet[
                    (value >> 18) & 0x3f]);

            output.push_back(
                Alphabet[
                    (value >> 12) & 0x3f]);

            output.push_back(
                hasSecond
                    ? Alphabet[
                          (value >> 6) & 0x3f]
                    : '=');

            output.push_back(
                hasThird
                    ? Alphabet[
                          value & 0x3f]
                    : '=');
        }

        return output;
    }

    bool ReadTextValue(
        const core::resources::DataSection& section,
        std::string& output)
    {
        output.clear();

        if (const std::string* value =
                section.AsString())
        {
            output =
                *value;
        }
        else if (const std::int64_t* value =
                     section.AsInteger())
        {
            output =
                std::to_string(
                    *value);
        }
        else if (const bool* value =
                     section.AsBoolean())
        {
            output =
                *value
                    ? "true"
                    : "false";
        }
        else if (const auto* value =
                     section.AsBinary())
        {
            output =
                EncodeBase64(
                    *value);
        }
        else
        {
            return false;
        }

        while (!output.empty() &&
               output.back() == '\0')
        {
            output.pop_back();
        }

        return
            !output.empty();
    }

    std::string PropertyKey(
        const core::assets::VisualMaterialProperty& property)
    {
        if (!property.name.empty())
        {
            return
                ToLower(
                    property.name);
        }

        return
            ToLower(
                EncodeBase64(
                    property.binaryName));
    }

    void MergeProperty(
        core::assets::VisualMaterial& material,
        core::assets::VisualMaterialProperty property)
    {
        const std::string key =
            PropertyKey(
                property);

        if (!key.empty())
        {
            for (core::assets::VisualMaterialProperty& current :
                 material.properties)
            {
                if (PropertyKey(
                        current) ==
                    key)
                {
                    current =
                        std::move(
                            property);

                    return;
                }
            }
        }

        material.properties.push_back(
            std::move(
                property));
    }

    bool ReadProperty(
        const core::resources::ResourceFileSystem& resources,
        const core::resources::DataSection& section,
        core::assets::VisualMaterialProperty& output,
        std::string& error)
    {
        output = {};

        ReadTextValue(
            section,
            output.name);

        if (const auto* binary =
                section.AsBinary())
        {
            output.binaryName =
                *binary;
        }

        if (const auto* textureSection =
                section.FindChild(
                    "Texture"))
        {
            std::string reference;

            if (!ReadTextValue(
                    *textureSection,
                    reference))
            {
                error =
                    "Material Texture property contains invalid reference.";

                return false;
            }

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
                    "Unable to resolve material texture: " +
                    reference;

                return false;
            }

            output.texture =
                std::move(
                    texture);
        }

        if (const auto* vectorSection =
                section.FindChild(
                    "Vector4"))
        {
            const auto* values =
                vectorSection->AsFloats();

            if (values == nullptr ||
                values->size() != 4)
            {
                error =
                    "Material Vector4 property is invalid.";

                return false;
            }

            output.vector4 =
                std::array<float, 4>
                {
                    (*values)[0],
                    (*values)[1],
                    (*values)[2],
                    (*values)[3]
                };
        }

        return true;
    }
}

namespace core::assets
{
    bool MaterialLoader::Load(
        const resources::ResourceFileSystem& resources,
        const resources::DataSection& section,
        VisualMaterial& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        return LoadSection(
            resources,
            section,
            0,
            output,
            error);
    }

    bool MaterialLoader::LoadReference(
        const resources::ResourceFileSystem& resources,
        const std::string_view reference,
        const std::size_t depth,
        VisualMaterial& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (reference.empty())
        {
            error =
                "MFM reference is empty.";

            return false;
        }

        std::string logicalPath =
            resources::ResourcePath::ToResPath(
                reference);

        if (logicalPath.empty())
        {
            error =
                "MFM reference path is invalid.";

            return false;
        }

        std::filesystem::path path(
            logicalPath);

        if (!path.has_extension())
        {
            logicalPath +=
                ".mfm";
        }

        logicalPath =
            resources::ResourcePath::Normalize(
                logicalPath);

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "MFM resource was not found: " +
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
                "Unable to read MFM resource: " +
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
                "Unable to parse MFM " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        if (!LoadSection(
                resources,
                root,
                depth,
                output,
                error))
        {
            error =
                logicalPath +
                ": " +
                error;

            return false;
        }

        return true;
    }

    bool MaterialLoader::LoadSection(
        const resources::ResourceFileSystem& resources,
        const resources::DataSection& section,
        const std::size_t depth,
        VisualMaterial& output,
        std::string& error) const
    {
        if (depth >
            MaximumInheritanceDepth)
        {
            error =
                "MFM inheritance depth exceeded.";

            return false;
        }

        VisualMaterial
            material;

        if (const resources::DataSection* mfm =
                section.FindChild(
                    "mfm"))
        {
            std::string reference;

            if (ReadTextValue(
                    *mfm,
                    reference))
            {
                VisualMaterial
                    inherited;

                std::string
                    inheritanceError;

                if (LoadReference(
                        resources,
                        reference,
                        depth + 1,
                        inherited,
                        inheritanceError))
                {
                    material =
                        std::move(
                            inherited);
                }
                else
                {
                    core::Log::Warning(
                        std::string(
                            "Material MFM fallback: ") +
                        inheritanceError);
                }
            }
        }

        if (const resources::DataSection* identifier =
                section.FindChild(
                    "identifier"))
        {
            ReadTextValue(
                *identifier,
                material.identifier);

            if (const auto* value =
                    identifier->AsBinary())
            {
                material.binaryIdentifier =
                    *value;
            }
        }

        if (const resources::DataSection* effect =
                section.FindChild(
                    "fx"))
        {
            std::string value;

            if (!ReadTextValue(
                    *effect,
                    value))
            {
                error =
                    "Material fx value is invalid.";

                return false;
            }

            material.effect =
                std::move(
                    value);
        }

        if (const resources::DataSection* collisionFlags =
                section.FindChild(
                    "collisionFlags"))
        {
            if (!ReadInt32(
                    *collisionFlags,
                    material.collisionFlags))
            {
                error =
                    "Material collisionFlags value is invalid.";

                return false;
            }
        }

        if (const resources::DataSection* materialKind =
                section.FindChild(
                    "materialKind"))
        {
            if (!ReadInt32(
                    *materialKind,
                    material.materialKind))
            {
                error =
                    "Material materialKind value is invalid.";

                return false;
            }
        }

        for (const resources::DataSection* property :
             section.FindChildren(
                 "property"))
        {
            VisualMaterialProperty
                value;

            if (!ReadProperty(
                    resources,
                    *property,
                    value,
                    error))
            {
                return false;
            }

            MergeProperty(
                material,
                std::move(
                    value));
        }

        output =
            std::move(
                material);

        return true;
    }
}
