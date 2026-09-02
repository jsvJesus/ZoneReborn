#include "Core/Assets/VisualLoader.h"
#include "Core/Assets/TextureResolver.h"

#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
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

    bool ReadTransform(
        const core::resources::DataSection& section,
        core::math::Transform3x4& output)
    {
        const auto* values =
            section.AsFloats();

        if (values == nullptr ||
            values->size() != 12)
        {
            return false;
        }

        for (std::size_t index = 0;
             index < 12;
             ++index)
        {
            output.values[index] =
                (*values)[index];
        }

        return true;
    }

    bool ReadVector3(
        const core::resources::DataSection& section,
        core::math::Vector3& output)
    {
        const auto* values =
            section.AsFloats();

        if (values == nullptr ||
            values->size() != 3)
        {
            return false;
        }

        output.x = (*values)[0];
        output.y = (*values)[1];
        output.z = (*values)[2];

        return true;
    }

    bool ReadBoundingBox(
        const core::resources::DataSection& section,
        core::math::BoundingBox& output)
    {
        const auto* minimum =
            section.FindChild("min");

        const auto* maximum =
            section.FindChild("max");

        if (minimum == nullptr ||
            maximum == nullptr)
        {
            return false;
        }

        return
            ReadVector3(*minimum, output.minimum) &&
            ReadVector3(*maximum, output.maximum);
    }

    bool ReadMaterialProperty(
        const core::resources::DataSection& section,
        core::assets::VisualMaterialProperty& output)
    {
        output = {};

        if (const std::string* name =
                section.AsString())
        {
            output.name = *name;
        }
        else if (const auto* binary =
                     section.AsBinary())
        {
            output.binaryName = *binary;
        }

        if (const auto* texture =
                section.FindChild("Texture"))
        {
            const std::string* value =
                texture->AsString();

            if (value == nullptr)
            {
                return false;
            }

            core::assets::TextureResource resource;

            resource.sourceReference =
                *value;

            resource.sourceLogicalPath =
                core::resources::ResourcePath::ToResPath(
                    *value);

            resource.logicalPath =
                resource.sourceLogicalPath;

            output.texture =
                std::move(resource);
        }

        if (const auto* vector =
                section.FindChild("Vector4"))
        {
            const auto* values =
                vector->AsFloats();

            if (values == nullptr ||
                values->size() != 4)
            {
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

    bool ReadMaterial(
        const core::resources::DataSection& section,
        core::assets::VisualMaterial& output)
    {
        output = {};

        if (const auto* identifier =
                section.FindChild("identifier"))
        {
            if (const std::string* value =
                    identifier->AsString())
            {
                output.identifier = *value;
            }
            else if (const auto* value =
                         identifier->AsBinary())
            {
                output.binaryIdentifier = *value;
            }
        }

        if (const auto* effect =
                section.FindChild("fx"))
        {
            const std::string* value =
                effect->AsString();

            if (value == nullptr)
            {
                return false;
            }

            output.effect = *value;
        }

        if (const auto* collisionFlags =
                section.FindChild("collisionFlags"))
        {
            if (!ReadInt32(
                    *collisionFlags,
                    output.collisionFlags))
            {
                return false;
            }
        }

        if (const auto* materialKind =
                section.FindChild("materialKind"))
        {
            if (!ReadInt32(
                    *materialKind,
                    output.materialKind))
            {
                return false;
            }
        }

        for (const auto* property :
             section.FindChildren("property"))
        {
            core::assets::VisualMaterialProperty value;

            if (!ReadMaterialProperty(
                    *property,
                    value))
            {
                return false;
            }

            output.properties.push_back(
                std::move(value));
        }

        return true;
    }

    bool ReadPrimitiveGroup(
        const core::resources::DataSection& section,
        core::assets::VisualPrimitiveGroup& output)
    {
        output = {};

        if (!ReadInt32(
                section,
                output.index))
        {
            return false;
        }

        const auto* material =
            section.FindChild("material");

        if (material == nullptr)
        {
            return false;
        }

        return ReadMaterial(
            *material,
            output.material);
    }

    bool ReadGeometry(
        const core::resources::DataSection& section,
        core::assets::VisualGeometry& output)
    {
        output = {};

        const auto* vertices =
            section.FindChild("vertices");

        if (vertices == nullptr)
        {
            return false;
        }

        output.vertexSection =
            vertices->name;

        if (const auto* descriptor =
                vertices->AsBinary())
        {
            output.vertexDescriptor =
                *descriptor;
        }

        for (const auto* stream :
             section.FindChildren("stream"))
        {
            const std::string* value =
                stream->AsString();

            if (value == nullptr)
            {
                return false;
            }

            output.streams.push_back(
                *value);
        }

        const auto* primitive =
            section.FindChild("primitive");

        if (primitive == nullptr)
        {
            return false;
        }

        const std::string* primitiveName =
            primitive->AsString();

        if (primitiveName == nullptr)
        {
            return false;
        }

        output.primitiveSection =
            *primitiveName;

        for (const auto* primitiveGroup :
             section.FindChildren("primitiveGroup"))
        {
            core::assets::VisualPrimitiveGroup group;

            if (!ReadPrimitiveGroup(
                    *primitiveGroup,
                    group))
            {
                return false;
            }

            output.primitiveGroups.push_back(
                std::move(group));
        }

        return true;
    }

    bool ReadRenderSet(
        const core::resources::DataSection& section,
        core::assets::VisualRenderSet& output)
    {
        output = {};

        if (const auto* treatAsWorldSpaceObject =
                section.FindChild(
                    "treatAsWorldSpaceObject"))
        {
            const bool* value =
                treatAsWorldSpaceObject->AsBoolean();

            if (value == nullptr)
            {
                return false;
            }

            output.treatAsWorldSpaceObject =
                *value;
        }

        for (const auto* node :
             section.FindChildren("node"))
        {
            const std::string* value =
                node->AsString();

            if (value == nullptr)
            {
                return false;
            }

            output.nodes.push_back(
                *value);
        }

        for (const auto* geometry :
             section.FindChildren("geometry"))
        {
            core::assets::VisualGeometry value;

            if (!ReadGeometry(
                    *geometry,
                    value))
            {
                return false;
            }

            output.geometries.push_back(
                std::move(value));
        }

        return true;
    }
}

namespace core::assets
{
    bool VisualLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view visualReference,
        VisualAsset& output,
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
                visualReference);

        if (logicalPath.empty())
        {
            error =
                "Visual resource path is invalid.";

            return false;
        }

        std::vector<std::byte> data;

        if (!resources.ReadBinary(
                logicalPath,
                data))
        {
            error =
                "Unable to read visual: " +
                logicalPath;

            return false;
        }

        resources::PackedSectionReader reader;

        resources::DataSection root;

        if (!reader.Read(
                data,
                root,
                error))
        {
            error =
                "Unable to parse visual " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        VisualAsset visual;

        visual.logicalPath =
            logicalPath;

        for (const auto* node :
             root.FindChildren("node"))
        {
            VisualNode value;

            const auto* identifier =
                node->FindChild("identifier");

            const auto* transform =
                node->FindChild("transform");

            if (identifier == nullptr ||
                transform == nullptr)
            {
                error =
                    "Visual contains invalid node.";

                return false;
            }

            const std::string* name =
                identifier->AsString();

            if (name == nullptr)
            {
                error =
                    "Visual node contains invalid identifier.";

                return false;
            }

            value.identifier =
                *name;

            if (!ReadTransform(
                    *transform,
                    value.transform))
            {
                error =
                    "Visual node contains invalid transform.";

                return false;
            }

            visual.nodes.push_back(
                std::move(value));
        }

        for (const auto* renderSet :
             root.FindChildren("renderSet"))
        {
            VisualRenderSet value;

            if (!ReadRenderSet(
                    *renderSet,
                    value))
            {
                error =
                    "Visual contains invalid renderSet.";

                return false;
            }

            visual.renderSets.push_back(
                std::move(value));
        }

        if (const auto* boundingBox =
                root.FindChild("boundingBox"))
        {
            math::BoundingBox box;

            if (!ReadBoundingBox(
                    *boundingBox,
                    box))
            {
                error =
                    "Visual contains invalid boundingBox.";

                return false;
            }

            visual.boundingBox =
                box;
        }

        TextureResolver textureResolver;

        for (VisualRenderSet& renderSet :
             visual.renderSets)
        {
            for (VisualGeometry& geometry :
                 renderSet.geometries)
            {
                for (VisualPrimitiveGroup& group :
                     geometry.primitiveGroups)
                {
                    for (VisualMaterialProperty& property :
                         group.material.properties)
                    {
                        if (!property.texture.has_value())
                        {
                            continue;
                        }

                        TextureResource resolvedTexture;

                        if (!textureResolver.Resolve(
                                resources,
                                property.texture->sourceReference,
                                resolvedTexture))
                        {
                            error =
                                "Visual contains invalid texture reference: " +
                                property.texture->sourceReference;

                            return false;
                        }

                        property.texture =
                            std::move(resolvedTexture);
                    }
                }
            }
        }

        output =
            std::move(visual);

        return true;
    }
}