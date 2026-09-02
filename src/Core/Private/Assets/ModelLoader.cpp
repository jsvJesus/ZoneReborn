#include "Core/Assets/ModelLoader.h"

#include "Core/Assets/ModelSourceLoader.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <string>
#include <utility>

namespace
{
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
}

namespace core::assets
{
    bool ModelLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        ModelAsset& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        ModelSourceLoader sourceLoader;

        ModelSource source;

        if (!sourceLoader.Load(
                resources,
                modelReference,
                source,
                error))
        {
            return false;
        }

        resources::PackedSectionReader reader;

        resources::DataSection root;

        if (!reader.Read(
                source.data,
                root,
                error))
        {
            error =
                "Unable to parse model " +
                source.resource.logicalPath +
                ": " +
                error;

            return false;
        }

        ModelAsset model;

        model.resource =
            std::move(source.resource);

        if (const auto* extent =
                root.FindChild("extent"))
        {
            if (!extent->TryGetFloat(
                    model.extent))
            {
                error =
                    "Model contains invalid extent.";

                return false;
            }
        }

        if (const auto* batched =
                root.FindChild("batched"))
        {
            const bool* value =
                batched->AsBoolean();

            if (value == nullptr)
            {
                error =
                    "Model contains invalid batched value.";

                return false;
            }

            model.batched = *value;
        }

        if (const auto* visibilityBox =
                root.FindChild("visibilityBox"))
        {
            math::BoundingBox box;

            if (!ReadBoundingBox(
                    *visibilityBox,
                    box))
            {
                error =
                    "Model contains invalid visibilityBox.";

                return false;
            }

            model.visibilityBox =
                box;
        }

        const resources::DataSection* visualSection =
            root.FindChild("nodelessVisual");

        if (visualSection == nullptr)
        {
            visualSection =
                root.FindChild("nodefullVisual");
        }

        if (visualSection == nullptr)
        {
            visualSection =
                root.FindChild("billboardVisual");
        }

        if (visualSection == nullptr)
        {
            error =
                "Model does not contain a visual reference.";

            return false;
        }

        const std::string* visualReference =
            visualSection->AsString();

        if (visualReference == nullptr ||
            visualReference->empty())
        {
            error =
                "Model contains invalid visual reference.";

            return false;
        }

        model.visualReference =
            *visualReference;

        model.visualLogicalPath =
            resources::ResourcePath::ToResPath(
                model.visualReference +
                ".visual");

        model.primitivesLogicalPath =
            resources::ResourcePath::ToResPath(
                model.visualReference +
                ".primitives");

        if (model.visualLogicalPath.empty() ||
            model.primitivesLogicalPath.empty())
        {
            error =
                "Unable to build model resource paths.";

            return false;
        }

        model.visualExists =
            resources.Exists(
                model.visualLogicalPath);

        model.primitivesExists =
            resources.Exists(
                model.primitivesLogicalPath);

        output =
            std::move(model);

        return true;
    }
}