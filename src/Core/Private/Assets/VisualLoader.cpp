#include "Core/Assets/VisualLoader.h"
#include "Core/Assets/MaterialLoader.h"

#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <cstddef>
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

    bool ReadPrimitiveGroup(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::MaterialLoader& materialLoader,
        const core::resources::DataSection& section,
        core::assets::VisualPrimitiveGroup& output,
        std::string& error)
    {
        output = {};

        if (!ReadInt32(
                section,
                output.index))
        {
            error =
                "Visual contains invalid primitive group index.";

            return false;
        }

        const auto* material =
            section.FindChild(
                "material");

        if (material == nullptr)
        {
            error =
                "Visual primitive group does not contain material.";

            return false;
        }

        if (!materialLoader.Load(
                resources,
                *material,
                output.material,
                error))
        {
            return false;
        }

        return true;
    }

    bool ReadGeometry(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::MaterialLoader& materialLoader,
        const core::resources::DataSection& section,
        core::assets::VisualGeometry& output,
        std::string& error)
    {
        output = {};

        const auto* vertices =
            section.FindChild("vertices");

        if (vertices == nullptr)
        {
            return false;
        }

        output.vertexSection.clear();
        output.vertexDescriptor.clear();

        if (const std::string* vertexSection =
                vertices->AsString();
            vertexSection != nullptr &&
            !vertexSection->empty())
        {
            output.vertexSection =
                *vertexSection;
        }
        else
        {
            output.vertexSection =
                vertices->name;

            if (const auto* descriptor =
                    vertices->AsBinary())
            {
                output.vertexDescriptor =
                    *descriptor;
            }
        }

        if (output.vertexSection.empty())
        {
            return false;
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
                resources,
                materialLoader,
                *primitiveGroup,
                group,
                error))
            {
                return false;
            }

            output.primitiveGroups.push_back(
                std::move(group));
        }

        return true;
    }

    std::string EncodeBase64(
        const core::resources::DataSection::BinaryData& data)
    {
        static constexpr char Alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string output;

        output.reserve(
            (
                data.size() +
                2
            ) /
            3 *
            4);

        for (std::size_t index = 0;
             index < data.size();
             index += 3)
        {
            const std::size_t remaining =
                data.size() -
                index;

            const unsigned int first =
                std::to_integer<unsigned char>(
                    data[index]);

            const unsigned int second =
                remaining > 1
                    ? std::to_integer<unsigned char>(
                        data[index + 1])
                    : 0u;

            const unsigned int third =
                remaining > 2
                    ? std::to_integer<unsigned char>(
                        data[index + 2])
                    : 0u;

            const unsigned int block =
                (first << 16u) |
                (second << 8u) |
                third;

            output.push_back(
                Alphabet[
                    (block >> 18u) &
                    0x3Fu]);

            output.push_back(
                Alphabet[
                    (block >> 12u) &
                    0x3Fu]);

            output.push_back(
                remaining > 1
                    ? Alphabet[
                        (block >> 6u) &
                        0x3Fu]
                    : '=');

            output.push_back(
                remaining > 2
                    ? Alphabet[
                        block &
                        0x3Fu]
                    : '=');
        }

        return output;
    }

    bool ReadTextValue(
    const core::resources::DataSection& section,
    std::string& output)
    {
        output.clear();

        //
        // Обычная PackedSection string.
        //
        if (const std::string* value =
                section.AsString();
            value != nullptr)
        {
            output =
                *value;
        }
        //
        // Некоторые старые SO/BigWorld ресурсы могут
        // приезжать как integer, хотя логически поле
        // используется как текстовое.
        //
        else if (const std::int64_t* value =
                     section.AsInteger();
                 value != nullptr)
        {
            output =
                std::to_string(
                    *value);
        }
        //
        // То же самое для boolean.
        //
        else if (const bool* value =
                     section.AsBoolean();
                 value != nullptr)
        {
            output =
                *value
                    ? "true"
                    : "false";
        }
        //
        // И главное для старых packed ресурсов:
        // строка иногда может оказаться raw/blob.
        //
        else if (const auto* value =
                     section.AsBinary();
                 value != nullptr)
        {
            output =
                EncodeBase64(
                    *value);
        }
        else
        {
            return false;
        }

        //
        // На случай если PackedSection string
        // содержит \0 внутри размера descriptor.
        //
        while (!output.empty() &&
               output.back() == '\0')
        {
            output.pop_back();
        }

        return !output.empty();
    }

    bool ReadNodeRecursive(
        const core::resources::DataSection& section,
        const std::int32_t parentIndex,
        core::assets::VisualAsset& visual,
        std::string& error)
    {
        const auto* identifier =
            section.FindChild(
                "identifier");

        const auto* transform =
            section.FindChild(
                "transform");

        if (identifier == nullptr ||
            transform == nullptr)
        {
            error =
                "Visual contains invalid node.";

            return false;
        }

        std::string name;

        if (!ReadTextValue(
                *identifier,
                name))
        {
            error =
                "Visual node contains invalid identifier. "
                "DataSection value type index=" +
                std::to_string(
                    identifier->value.index());

            return false;
        }

        if (visual.nodes.size() >
            static_cast<std::size_t>(
                std::numeric_limits<
                    std::int32_t>::max()))
        {
            error =
                "Visual contains too many nodes.";

            return false;
        }

        core::assets::VisualNode
            node;

        node.identifier =
            std::move(
                name);

        node.parentIndex =
            parentIndex;

        if (!ReadTransform(
                *transform,
                node.transform))
        {
            error =
                "Visual node contains invalid transform.";

            return false;
        }

        const std::int32_t
            currentIndex =
                static_cast<std::int32_t>(
                    visual.nodes.size());

        visual.nodes.push_back(
            std::move(
                node));

        //
        // ВАЖНО:
        // старый loader читал только root node.
        // BigWorld skeleton находится внутри:
        //
        // node
        //   node
        //     node
        //       ...
        //
        for (const auto* child :
             section.FindChildren(
                 "node"))
        {
            if (child == nullptr)
            {
                continue;
            }

            if (!ReadNodeRecursive(
                    *child,
                    currentIndex,
                    visual,
                    error))
            {
                return false;
            }
        }

        return true;
    }

    bool ReadRenderSet(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::MaterialLoader& materialLoader,
        const core::resources::DataSection& section,
        core::assets::VisualRenderSet& output,
        std::string& error)
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
            if (node == nullptr)
            {
                continue;
            }

            std::string value;

            if (!ReadTextValue(
                    *node,
                    value))
            {
                error =
                    "Visual renderSet contains invalid node reference. "
                    "DataSection value type index=" +
                    std::to_string(
                        node->value.index());

                return false;
            }

            output.nodes.push_back(
                std::move(
                    value));
        }

        for (const auto* geometry :
             section.FindChildren("geometry"))
        {
            core::assets::VisualGeometry value;

            if (!ReadGeometry(
                resources,
                materialLoader,
                *geometry,
                value,
                error))
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

        MaterialLoader
            materialLoader;

        for (const auto* node :
            root.FindChildren(
         "node"))
        {
            if (node == nullptr)
            {
                continue;
            }

            if (!ReadNodeRecursive(
                    *node,
                    -1,
                    visual,
                    error))
            {
                return false;
            }
        }

        for (const auto* renderSet :
             root.FindChildren("renderSet"))
        {
            VisualRenderSet value;

            if (!ReadRenderSet(
                resources,
                materialLoader,
                *renderSet,
                value,
                error))
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

        output =
            std::move(visual);

        return true;
    }
}
