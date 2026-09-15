#include "Core/Assets/ModelLoader.h"

#include "Core/Assets/ModelSourceLoader.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr std::size_t MaximumParentDepth =
        16;

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
            section.FindChild(
                "min");

        const auto* maximum =
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

    const std::string*
    ReadReference(
        const core::resources::DataSection* section)
    {
        if (section == nullptr)
        {
            return nullptr;
        }

        if (const std::string* value =
                section->AsString();
            value != nullptr &&
            !value->empty())
        {
            return value;
        }

        const core::resources::DataSection* child =
            section->FindChild(
                "resource");

        if (child != nullptr)
        {
            if (const std::string* value =
                    child->AsString();
                value != nullptr &&
                !value->empty())
            {
                return value;
            }
        }

        child =
            section->FindChild(
                "visual");

        if (child != nullptr)
        {
            if (const std::string* value =
                    child->AsString();
                value != nullptr &&
                !value->empty())
            {
                return value;
            }
        }

        child =
            section->FindChild(
                "name");

        if (child != nullptr)
        {
            if (const std::string* value =
                    child->AsString();
                value != nullptr &&
                !value->empty())
            {
                return value;
            }
        }

        return nullptr;
    }

    std::string NormalizeVisualReference(
        const std::string_view reference)
    {
        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            return {};
        }

        if (normalized.starts_with(
                "res/"))
        {
            normalized.erase(
                0,
                4);
        }

        if (normalized.ends_with(
                ".visual"))
        {
            normalized.resize(
                normalized.size() -
                std::string_view(
                    ".visual").size());
        }
        else if (normalized.ends_with(
                     ".primitives"))
        {
            normalized.resize(
                normalized.size() -
                std::string_view(
                    ".primitives").size());
        }

        return normalized;
    }

    std::string NormalizeModelReference(
        const std::string_view reference)
    {
        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            return {};
        }

        if (normalized.starts_with(
                "res/"))
        {
            normalized.erase(
                0,
                4);
        }

        if (!normalized.ends_with(
                ".model"))
        {
            normalized +=
                ".model";
        }

        return normalized;
    }

    bool ApplyVisualReference(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view reference,
        core::assets::ModelAsset& model)
    {
        const std::string normalized =
            NormalizeVisualReference(
                reference);

        if (normalized.empty())
        {
            return false;
        }

        model.visualReference =
            normalized;

        model.visualLogicalPath =
            core::resources::ResourcePath::ToResPath(
                normalized +
                ".visual");

        model.primitivesLogicalPath =
            core::resources::ResourcePath::ToResPath(
                normalized +
                ".primitives");

        if (model.visualLogicalPath.empty() ||
            model.primitivesLogicalPath.empty())
        {
            return false;
        }

        model.visualExists =
            resources.Exists(
                model.visualLogicalPath);

        model.primitivesExists =
            resources.Exists(
                model.primitivesLogicalPath);

        return true;
    }

    bool TrySiblingVisual(
        const core::resources::ResourceFileSystem& resources,
        const std::string& modelLogicalPath,
        core::assets::ModelAsset& model)
    {
        std::string base =
            core::resources::ResourcePath::Normalize(
                modelLogicalPath);

        if (base.empty() ||
            !base.ends_with(
                ".model"))
        {
            return false;
        }

        base.resize(
            base.size() -
            std::string_view(
                ".model").size());

        const std::string visualPath =
            base +
            ".visual";

        const std::string primitivesPath =
            base +
            ".primitives";

        if (!resources.Exists(
                visualPath) ||
            !resources.Exists(
                primitivesPath))
        {
            return false;
        }

        std::string reference =
            base;

        if (reference.starts_with(
                "res/"))
        {
            reference.erase(
                0,
                4);
        }

        model.visualReference =
            reference;

        model.visualLogicalPath =
            visualPath;

        model.primitivesLogicalPath =
            primitivesPath;

        model.visualExists =
            true;

        model.primitivesExists =
            true;

        return true;
    }

    bool LoadModelRecursive(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        const std::size_t depth,
        std::unordered_set<std::string>& visited,
        core::assets::ModelAsset& output,
        std::string& error)
    {
        output = {};
        error.clear();

        if (depth >
            MaximumParentDepth)
        {
            error =
                "Model parent chain exceeds maximum depth.";

            return false;
        }

        core::assets::ModelSourceLoader
            sourceLoader;

        core::assets::ModelSource
            source;

        if (!sourceLoader.Load(
                resources,
                modelReference,
                source,
                error))
        {
            return false;
        }

        if (!visited.insert(
                source.resource.logicalPath).second)
        {
            error =
                "Model parent cycle detected: " +
                source.resource.logicalPath;

            return false;
        }

        core::resources::PackedSectionReader
            reader;

        core::resources::DataSection
            root;

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

        core::assets::ModelAsset
            model;

        model.resource =
            source.resource;

        if (const auto* extent =
                root.FindChild(
                    "extent"))
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
                root.FindChild(
                    "batched"))
        {
            const bool* value =
                batched->AsBoolean();

            if (value == nullptr)
            {
                error =
                    "Model contains invalid batched value.";

                return false;
            }

            model.batched =
                *value;
        }

        if (const auto* visibilityBox =
                root.FindChild(
                    "visibilityBox"))
        {
            core::math::BoundingBox
                box;

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

        const core::resources::DataSection* visualSection =
            root.FindChild(
                "nodelessVisual");

        if (visualSection == nullptr)
        {
            visualSection =
                root.FindChild(
                    "nodefullVisual");
        }

        if (visualSection == nullptr)
        {
            visualSection =
                root.FindChild(
                    "billboardVisual");
        }

        if (const std::string* visualReference =
                ReadReference(
                    visualSection);
            visualReference != nullptr)
        {
            if (ApplyVisualReference(
                    resources,
                    *visualReference,
                    model))
            {
                output =
                    std::move(
                        model);

                return true;
            }
        }

        const core::resources::DataSection* parentSection =
            root.FindChild(
                "parent");

        if (const std::string* parentReference =
                ReadReference(
                    parentSection);
            parentReference != nullptr)
        {
            const std::string normalizedParent =
                NormalizeModelReference(
                    *parentReference);

            if (!normalizedParent.empty())
            {
                core::assets::ModelAsset
                    parent;

                std::string parentError;

                if (LoadModelRecursive(
                        resources,
                        normalizedParent,
                        depth + 1,
                        visited,
                        parent,
                        parentError))
                {
                    model.visualReference =
                        parent.visualReference;

                    model.visualLogicalPath =
                        parent.visualLogicalPath;

                    model.primitivesLogicalPath =
                        parent.primitivesLogicalPath;

                    model.visualExists =
                        parent.visualExists;

                    model.primitivesExists =
                        parent.primitivesExists;

                    output =
                        std::move(
                            model);

                    return true;
                }
            }
        }

        if (TrySiblingVisual(
                resources,
                source.resource.logicalPath,
                model))
        {
            output =
                std::move(
                    model);

            return true;
        }

        if (visualSection !=
            nullptr)
        {
            error =
                "Model contains invalid visual reference.";
        }
        else
        {
            error =
                "Model does not contain a visual reference.";
        }

        return false;
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

        std::unordered_set<std::string>
            visited;

        return LoadModelRecursive(
            resources,
            modelReference,
            0,
            visited,
            output,
            error);
    }
}