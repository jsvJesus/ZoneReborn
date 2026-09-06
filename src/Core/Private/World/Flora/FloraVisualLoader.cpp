#include "Core/World/Flora/FloraVisualLoader.h"

#include "Core/Assets/PrimitivesLoader.h"
#include "Core/Assets/VisualLoader.h"
#include "Core/Resources/ResourcePath.h"

#include <string>
#include <string_view>
#include <utility>

namespace
{
    bool BuildVisualPaths(
        const std::string_view reference,
        std::string& visualPath,
        std::string& primitivesPath)
    {
        visualPath =
            core::resources::ResourcePath::ToResPath(
                reference);

        if (visualPath.empty())
        {
            return false;
        }

        if (!visualPath.ends_with(
                ".visual"))
        {
            visualPath +=
                ".visual";
        }

        primitivesPath =
            visualPath;

        constexpr std::string_view VisualExtension =
            ".visual";

        primitivesPath.resize(
            primitivesPath.size() -
            VisualExtension.size());

        primitivesPath +=
            ".primitives";

        return true;
    }
}

namespace core::world::flora
{
    bool FloraVisualLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view visualReference,
        FloraVisualAsset& output,
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

        FloraVisualAsset asset;

        asset.visualReference =
            std::string(
                visualReference);

        if (!BuildVisualPaths(
                visualReference,
                asset.visualLogicalPath,
                asset.primitivesLogicalPath))
        {
            error =
                "Invalid SO flora visual reference: " +
                std::string(
                    visualReference);

            return false;
        }

        if (!resources.Exists(
                asset.visualLogicalPath))
        {
            error =
                "SO flora visual was not found: " +
                asset.visualLogicalPath;

            return false;
        }

        if (!resources.Exists(
                asset.primitivesLogicalPath))
        {
            error =
                "SO flora primitives were not found: " +
                asset.primitivesLogicalPath;

            return false;
        }

        assets::VisualLoader
            visualLoader;

        if (!visualLoader.Load(
                resources,
                asset.visualLogicalPath,
                asset.visual,
                error))
        {
            error =
                "Unable to load SO flora visual " +
                asset.visualLogicalPath +
                ": " +
                error;

            return false;
        }

        assets::PrimitivesLoader
            primitivesLoader;

        if (!primitivesLoader.Load(
                resources,
                asset.primitivesLogicalPath,
                asset.primitives,
                error))
        {
            error =
                "Unable to load SO flora primitives " +
                asset.primitivesLogicalPath +
                ": " +
                error;

            return false;
        }

        if (asset.visual.renderSets.empty())
        {
            error =
                "SO flora visual contains no render sets: " +
                asset.visualLogicalPath;

            return false;
        }

        output =
            std::move(
                asset);

        return true;
    }
}